#include "YearInReading.h"

#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "BookReadingStats.h"
#include "DailyReadingStats.h"
#include "GlobalReadingStats.h"
#include "ReadingStatsUtils.h"
#include "ClippingStore.h"
#include "CrossPointSettings.h"
#include "fontIds.h"

namespace {
constexpr const char* kCacheRoot = "/.crosspoint";
constexpr uint32_t kUnixDaysBefore2000 = 10957;
// Quotes longer than this rarely fit the space left under the stats.
constexpr size_t kQuoteMinBytes = 24;
constexpr size_t kQuoteMaxBytes = 420;

bool hasPrefix(const char* name, const char* prefix) { return strncmp(name, prefix, strlen(prefix)) == 0; }

// Clipping timestamps are UTC seconds; the daily counters use local days.
bool localDayFromUnix(const uint32_t unixSeconds, uint32_t& day) {
  if (unixSeconds == 0) return false;
  const int offsetMinutes = (static_cast<int>(std::min<uint8_t>(SETTINGS.clockUtcOffsetQ, 104)) - 48) * 15;
  const int64_t local = static_cast<int64_t>(unixSeconds) + offsetMinutes * 60;
  const int64_t unixDay = local / 86400;
  if (unixDay < kUnixDaysBefore2000) return false;
  day = static_cast<uint32_t>(unixDay - kUnixDaysBefore2000);
  return true;
}

void flattenWhitespace(std::string& text) {
  for (char& c : text) {
    if (c == '\n' || c == '\r' || c == '\t') c = ' ';
  }
}

void formatHours(const uint32_t seconds, char* buf, const size_t len) {
  if (seconds < 3600) {
    BookReadingStats::formatDuration(seconds, buf, len);
    return;
  }
  snprintf(buf, len, "%lu h", static_cast<unsigned long>((seconds + 1800) / 3600));
}

int drawStatRow(const GfxRenderer& renderer, const int x, const int width, const int y, const char* label,
                const char* value) {
  const int valueH = renderer.getLineHeight(UI_12_FONT_ID);
  const int labelH = renderer.getLineHeight(UI_10_FONT_ID);
  const int valueW = renderer.getTextWidth(UI_12_FONT_ID, value, EpdFontFamily::BOLD);
  const std::string shownLabel = renderer.truncatedText(UI_10_FONT_ID, label, width - valueW - 16);
  renderer.drawText(UI_10_FONT_ID, x, y + (valueH - labelH) / 2, shownLabel.c_str());
  renderer.drawText(UI_12_FONT_ID, x + width - valueW, y, value, true, EpdFontFamily::BOLD);
  return valueH;
}
}  // namespace

bool YearInReading::load() {
  ReadingStatsDateTime now;
  if (!halClock.isAvailable() || !getCurrentLocalDailyReadingDateTime(now)) return false;
  const uint32_t today = readingStatsDayIndex(now.date);
  // The first two weeks of January still look back on the year just ended.
  year = now.date.year;
  if (now.date.month == 1 && now.date.day <= 14) --year;
  ReadingStatsDate firstDate;
  firstDate.year = year;
  firstDate.month = 1;
  firstDate.day = 1;
  ReadingStatsDate lastDate;
  lastDate.year = year;
  lastDate.month = 12;
  lastDate.day = 31;
  const uint32_t first = readingStatsDayIndex(firstDate);
  const uint32_t last = std::min(today, readingStatsDayIndex(lastDate));

  uint64_t seconds = 0;
  if (DailyReadingStats::sumSeconds(first, last, seconds)) {
    readingSeconds = static_cast<uint32_t>(std::min<uint64_t>(seconds, UINT32_MAX));
  }

  // The reading calendar keeps the last two years, one bit per day.
  const GlobalReadingStats global = GlobalReadingStats::load();
  const bool hasHistory = global.readingHistoryAnchorDay != 0 || (global.readingHistoryBits[0] & 1u);
  uint16_t run = 0;
  for (uint32_t day = first; hasHistory && day <= last; ++day) {
    bool read = false;
    if (day <= global.readingHistoryAnchorDay) {
      const uint32_t delta = global.readingHistoryAnchorDay - day;
      read = delta < READING_HISTORY_DAYS && (global.readingHistoryBits[delta / 8] & (1u << (delta % 8))) != 0;
    }
    if (read) {
      ++daysRead;
      longestStreak = std::max<uint16_t>(longestStreak, ++run);
    } else {
      run = 0;
    }
  }

  // Finished books: every EPUB/XTC cache folder with a finished date this year.
  auto dir = Storage.open(kCacheRoot);
  if (dir && dir.isDirectory()) {
    while (true) {
      auto entry = dir.openNextFile();
      if (!entry) break;
      char name[64]{};
      const size_t nameLength = entry.getName(name, sizeof(name));
      const bool folder = entry.isDirectory();
      entry.close();
      if (!folder || nameLength == 0 || nameLength >= sizeof(name) - 1) continue;
      if (!hasPrefix(name, "epub_") && !hasPrefix(name, "xtc_")) continue;
      BookReadingStats stats;
      if (BookReadingStats::loadForUpload(std::string(kCacheRoot) + "/" + name, stats) && stats.isCompleted &&
          stats.finishedDate.year == year) {
        ++booksFinished;
      }
    }
  }
  if (dir) dir.close();

  // Highlights made this year, the book with the most, and one of them as the
  // quote. The pick is seeded by the day, so the quote changes once a day.
  std::vector<ClippedBookEntry> books;
  ClippingStore::getAllClippedBooks(books);
  uint32_t seed = today * 2654435761u + 1;
  uint32_t eligible = 0;
  for (const ClippedBookEntry& book : books) {
    if (book.bookType != "epub") continue;
    uint16_t bookCount = 0;
    for (size_t ordinal = 0; ordinal < CLIPPING_MAX_PER_BOOK; ++ordinal) {
      Clipping clipping;
      std::string text;
      bool done = false;
      if (!ClippingStore::readForUpload(book.bookPath, ordinal, clipping, text, done) || done) break;
      uint32_t day = 0;
      if (!localDayFromUnix(clippingUnixTimestamp(clipping.timestamp), day) || day < first || day > last) continue;
      ++bookCount;
      if (text.size() < kQuoteMinBytes || text.size() > kQuoteMaxBytes) continue;
      ++eligible;
      seed = seed * 1664525u + 1013904223u;
      if ((seed >> 8) % eligible == 0) {
        quote = std::move(text);
        quoteBookTitle = book.bookTitle;
      }
    }
    highlights += bookCount;
    if (bookCount > topBookHighlights) {
      topBookHighlights = bookCount;
      topBookTitle = book.bookTitle;
      topBookAuthor = book.bookAuthor;
    }
  }
  flattenWhitespace(quote);
  LOG_INF("YEAR", "%u: %lu books, %lus, %u days, streak %u, %lu highlights", year,
          static_cast<unsigned long>(booksFinished), static_cast<unsigned long>(readingSeconds), daysRead,
          longestStreak, static_cast<unsigned long>(highlights));
  return true;
}

void YearInReading::draw(const GfxRenderer& renderer, const int x, const int top, const int width,
                         const int height) const {
  const int margin = x;
  const int contentW = width;
  const int bottom = top + height;
  const int smallH = renderer.getLineHeight(SMALL_FONT_ID);
  const int uiH = renderer.getLineHeight(UI_10_FONT_ID);
  const int bigH = renderer.getLineHeight(UI_12_FONT_ID);
  const int rowGap = std::max(12, height / 55);
  const int pageW = x * 2 + width;
  char buf[48];

  int y = top + rowGap / 2;
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(booksFinished));
  y += drawStatRow(renderer, margin, contentW, y, tr(STR_LIBRARY_FINISHED_BOOKS), buf) + rowGap;
  formatHours(readingSeconds, buf, sizeof(buf));
  y += drawStatRow(renderer, margin, contentW, y, tr(STR_STATS_TIME_LBL), buf) + rowGap;
  snprintf(buf, sizeof(buf), "%u", daysRead);
  y += drawStatRow(renderer, margin, contentW, y, tr(STR_YEAR_DAYS_READ), buf) + rowGap;
  snprintf(buf, sizeof(buf), "%u %s", longestStreak, tr(STR_STATS_DAYS));
  y += drawStatRow(renderer, margin, contentW, y, tr(STR_STATS_LONGEST_STREAK_LBL), buf) + rowGap;
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(highlights));
  y += drawStatRow(renderer, margin, contentW, y, tr(STR_YEAR_HIGHLIGHTS), buf) + rowGap;

  if (topBookHighlights > 0) {
    renderer.drawLine(margin, y, pageW - margin, y, 1, true);
    y += rowGap;
    renderer.drawText(SMALL_FONT_ID, margin, y, tr(STR_YEAR_MOST_HIGHLIGHTED));
    y += smallH + 6;
    for (const auto& line : renderer.wrappedText(UI_12_FONT_ID, topBookTitle.c_str(), contentW, 2, EpdFontFamily::BOLD)) {
      renderer.drawText(UI_12_FONT_ID, margin, y, line.c_str(), true, EpdFontFamily::BOLD);
      y += bigH;
    }
    snprintf(buf, sizeof(buf), "%u", topBookHighlights);
    const int countW = renderer.getTextWidth(UI_10_FONT_ID, buf, EpdFontFamily::BOLD);
    if (!topBookAuthor.empty()) {
      const std::string author = renderer.truncatedText(UI_10_FONT_ID, topBookAuthor.c_str(), contentW - countW - 16);
      renderer.drawText(UI_10_FONT_ID, margin, y + 2, author.c_str());
    }
    renderer.drawText(UI_10_FONT_ID, margin + contentW - countW, y + 2, buf, true, EpdFontFamily::BOLD);
    y += uiH + rowGap;
  }

  if (!quote.empty()) {
    renderer.drawLine(margin, y, pageW - margin, y, 1, true);
    y += rowGap + 4;
    const int creditH = uiH + rowGap;
    const int lines = std::max(1, (bottom - creditH - y) / bigH);
    const std::string quoted = "\xE2\x80\x9C" + quote + "\xE2\x80\x9D";
    for (const auto& line : renderer.wrappedText(UI_12_FONT_ID, quoted.c_str(), contentW, std::min(lines, 12))) {
      renderer.drawText(UI_12_FONT_ID, margin, y, line.c_str());
      y += bigH;
    }
    y += 8;
    const std::string credit = "\xE2\x80\x94 " + quoteBookTitle;
    const std::string shown = renderer.truncatedText(UI_10_FONT_ID, credit.c_str(), contentW);
    const int creditW = renderer.getTextWidth(UI_10_FONT_ID, shown.c_str());
    renderer.drawText(UI_10_FONT_ID, margin + contentW - creditW, y, shown.c_str());
  }
}
