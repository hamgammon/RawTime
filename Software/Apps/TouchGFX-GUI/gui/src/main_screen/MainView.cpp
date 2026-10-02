#include <gui/main_screen/MainView.hpp>
#include <gui/common/RawTimeLabels.hpp>
#include <touchgfx/Color.hpp>

/// The face is square and the clock group is centred on it.
static const int16_t kFaceWidth = 240;

static const char* const kDayNames[7] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static const char* const kMonthNames[12] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

MainView::MainView()
    : mShown{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }  // nothing on screen yet, so the first reading draws
    , mStyle{ false, false }
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // Hide unused template elements
    dayText.setVisible(false);
    dayText.invalidate();
    monthText.setVisible(false);
    monthText.invalidate();
    stepsText.setVisible(false);
    stepsText.invalidate();
    rule.setVisible(false);
    rule.invalidate();
    stepsIcon.setVisible(false);
    stepsIcon.invalidate();

    // Date on one line: rebind weekdayText to larger buffer mDateBuffer
    mDateBuffer[0] = 0;
    weekdayText.setWildcard(mDateBuffer);
    weekdayText.setColor(touchgfx::Color::getColorFromRGB(192, 192, 192));

    // High-contrast, bold time typography
    hourText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    colonText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    minuteText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    meridiemText.setColor(touchgfx::Color::getColorFromRGB(180, 180, 180));

    // Construct battery indicator widget at bottom centre (x = 99, y = 206, w = 42, h = 16)
    if (mBatteryContainer.getParent() == nullptr) {
        mBatteryContainer.setPosition(99, 206, 42, 16);

        // Battery outer border
        mBatteryBody.setPosition(0, 0, 38, 16);
        mBatteryBody.setColor(touchgfx::Color::getColorFromRGB(140, 140, 140));
        mBatteryContainer.add(mBatteryBody);

        // Battery inner cavity (black background)
        mBatteryInner.setPosition(2, 2, 34, 12);
        mBatteryInner.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
        mBatteryContainer.add(mBatteryInner);

        // Battery terminal nub on the right
        mBatteryNub.setPosition(38, 5, 3, 6);
        mBatteryNub.setColor(touchgfx::Color::getColorFromRGB(140, 140, 140));
        mBatteryContainer.add(mBatteryNub);

        // 4 interior battery charge level segments
        for (int i = 0; i < 4; ++i) {
            mBatterySegments[i].setPosition(4 + i * 8, 4, 6, 8);
            mBatterySegments[i].setColor(touchgfx::Color::getColorFromRGB(40, 40, 40));
            mBatteryContainer.add(mBatterySegments[i]);
        }

        add(mBatteryContainer);
    }

    mStyle = presenter->clockStyle();
    layoutDate();
    setTime(presenter->currentTime());
    setBatteryLevel(presenter->batteryLevel());
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::place(touchgfx::TextArea &area, int16_t x, int16_t y,
                     int16_t width, int16_t height)
{
    area.invalidate();
    area.setPosition(x, y, width, height);
    area.invalidate();
}

void MainView::layoutClock()
{
    const int16_t hourWidth   = static_cast<int16_t>(hourText.getTextWidth());
    const int16_t minuteWidth = static_cast<int16_t>(minuteText.getTextWidth());
    const int16_t colonWidth  = static_cast<int16_t>(colonText.getTextWidth());

    int16_t total = static_cast<int16_t>(hourWidth + colonWidth + minuteWidth);
    if (mStyle.is12h) {
        total = static_cast<int16_t>(total + kMeridiemGap + meridiemText.getTextWidth());
    }

    int16_t x = static_cast<int16_t>((kFaceWidth - total) / 2);

    place(hourText, x, kClockY, hourWidth, kClockHeight);
    x = static_cast<int16_t>(x + hourWidth);

    place(colonText, x, kClockY, colonWidth, kClockHeight);
    x = static_cast<int16_t>(x + colonWidth);

    place(minuteText, x, kClockY, minuteWidth, kClockHeight);
    x = static_cast<int16_t>(x + minuteWidth + kMeridiemGap);

    if (mStyle.is12h) {
        place(meridiemText, x, static_cast<int16_t>(kClockY + kMeridiemDrop),
              static_cast<int16_t>(meridiemText.getTextWidth()), kMeridiemHeight);
        meridiemText.setVisible(true);
        meridiemText.invalidate();
    } else {
        meridiemText.setVisible(false);
        meridiemText.invalidate();
    }

    colonText.setVisible(true);
    colonText.invalidate();
}

void MainView::layoutDate()
{
    // Full date rendered on a single line, centered horizontally across the display
    place(weekdayText, 0, kDateY, kFaceWidth, kDateHeight);
}

void MainView::updateClockText()
{
    if (mStyle.is12h) {
        const uint8_t hour12 = (mShown.hour % 12u) == 0u ? 12u : (mShown.hour % 12u);
        Unicode::snprintf(hourTextBuffer, HOURTEXT_SIZE, "%u",
                          static_cast<unsigned>(hour12));

        meridiemText.setTypedText(touchgfx::TypedText(
            App::Labels::kMeridiemLabels[mShown.hour >= 12u ? 1 : 0]));
    } else {
        Unicode::snprintf(hourTextBuffer, HOURTEXT_SIZE, "%02u",
                          static_cast<unsigned>(mShown.hour));
    }

    Unicode::snprintf(minuteTextBuffer, MINUTETEXT_SIZE, "%02u",
                      static_cast<unsigned>(mShown.minute));
}

void MainView::updateDateText()
{
    const char* dayName = kDayNames[mShown.wday % 7u];
    const char* monthName = kMonthNames[mShown.mon % 12u];

    if (mStyle.monthFirst) {
        Unicode::snprintf(mDateBuffer, DATE_BUFFER_SIZE, "%s, %s %02u",
                          dayName, monthName, static_cast<unsigned>(mShown.mday));
    } else {
        Unicode::snprintf(mDateBuffer, DATE_BUFFER_SIZE, "%s, %02u %s",
                          dayName, static_cast<unsigned>(mShown.mday), monthName);
    }
    weekdayText.invalidate();
}

void MainView::setTime(const WallTime &time)
{
    if (time == mShown) {
        return;
    }

    mShown = time;

    updateClockText();
    layoutClock();
    updateDateText();
}

void MainView::setClockStyle(const ClockStyle &style)
{
    if (style == mStyle) {
        return;
    }

    mStyle = style;

    updateClockText();
    layoutClock();
    updateDateText();
}

void MainView::setSteps(uint32_t /*steps*/)
{
    // Steps are not drawn on RawTime watchface
}

void MainView::setBatteryLevel(uint8_t level)
{
    mBatteryLevel = level;

    // Segment thresholds matching UNA convention:
    //  0 %       0 segments
    //  1-24 %    1 segment (red low-battery alert)
    //  25-49 %   2 segments (teal)
    //  50-74 %   3 segments (teal)
    //  75-100 %  4 segments (teal)
    uint8_t numActive = 0;
    if (level >= 75) {
        numActive = 4;
    } else if (level >= 50) {
        numActive = 3;
    } else if (level >= 25) {
        numActive = 2;
    } else if (level >= 1) {
        numActive = 1;
    }

    const touchgfx::colortype activeColor = (level < 25)
        ? touchgfx::Color::getColorFromRGB(220, 50, 50)     // Low battery warning red
        : touchgfx::Color::getColorFromRGB(0, 200, 140);    // Crisp UNA teal

    const touchgfx::colortype inactiveColor = touchgfx::Color::getColorFromRGB(40, 40, 40);

    for (int i = 0; i < 4; ++i) {
        mBatterySegments[i].setColor((i < numActive) ? activeColor : inactiveColor);
        mBatterySegments[i].invalidate();
    }

    mBatteryContainer.invalidate();
}
