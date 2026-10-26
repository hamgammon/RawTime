#include <gui/main_screen/MainView.hpp>
#include <gui/common/RawTimeLabels.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <texts/TypedTextDatabase.hpp>
#include <fonts/GeneratedFont.hpp>
#include <touchgfx/Color.hpp>
#include <cstdio>

extern touchgfx::GeneratedFont& getFont_Poppins_SemiBold_76_2bpp();
extern touchgfx::GeneratedFont& getFont_Poppins_SemiBold_48_2bpp();

/// The face is square and the clock group is centred on it.
static const int16_t kFaceWidth = 240;

static const char* const kDayNames[7] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char* const kMonthNames[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

MainView::MainView()
    : mShown{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }  // nothing on screen yet, so the first reading draws
    , mStyle{ false, false, false }
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

    // Construct seconds colon and digits widgets (for optional HH:MM:SS mode)
    if (mColon2Text.getParent() == nullptr) {
        mColon2Text.setPosition(0, kClockSecondsY, 12, kClockSecondsHeight);
        mColon2Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        mColon2Text.setLinespacing(0);
        mColon2Text.setTypedText(touchgfx::TypedText(T_TEXT_CONSOLE_COLON));
        mColon2Text.setVisible(false);
        add(mColon2Text);
    }

    if (mSecondText.getParent() == nullptr) {
        mSecondText.setPosition(0, kClockSecondsY, 64, kClockSecondsHeight);
        mSecondText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        mSecondText.setLinespacing(0);
        Unicode::snprintf(mSecondTextBuffer, SECONDTEXT_SIZE, "%s",
                          touchgfx::TypedText(T_WC_CONSOLE_MINUTE).getText());
        mSecondText.setWildcard(mSecondTextBuffer);
        mSecondText.setTypedText(touchgfx::TypedText(T_TMP_CONSOLE_CLOCK));
        mSecondText.setVisible(false);
        add(mSecondText);
    }

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
    setBattery(presenter->batteryLevel(), presenter->isBatteryEnabled());
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
    hourText.invalidate();
    colonText.invalidate();
    minuteText.invalidate();
    mColon2Text.invalidate();
    mSecondText.invalidate();
    meridiemText.invalidate();

    // Font index 1 is used by T_TMP_CONSOLE_CLOCK and T_TEXT_CONSOLE_COLON.
    // Switch to 48pt when showing seconds so HH:MM:SS fits on one line, or 76pt for HH:MM.
    TypedTextDatabase::setFont(1, mStyle.showSeconds
        ? &(getFont_Poppins_SemiBold_48_2bpp())
        : &(getFont_Poppins_SemiBold_76_2bpp()));

    const int16_t clockY      = mStyle.showSeconds ? kClockSecondsY : kClockY;
    const int16_t clockHeight = mStyle.showSeconds ? kClockSecondsHeight : kClockHeight;

    const int16_t hourWidth   = static_cast<int16_t>(hourText.getTextWidth());
    const int16_t minuteWidth = static_cast<int16_t>(minuteText.getTextWidth());
    const int16_t colonWidth  = static_cast<int16_t>(colonText.getTextWidth());

    int16_t colon2Width = 0;
    int16_t secondWidth = 0;
    if (mStyle.showSeconds) {
        colon2Width = static_cast<int16_t>(mColon2Text.getTextWidth());
        secondWidth = static_cast<int16_t>(mSecondText.getTextWidth());
    }

    const int16_t digitsTotal = static_cast<int16_t>(
        hourWidth + colonWidth + minuteWidth + colon2Width + secondWidth);

    int16_t total = digitsTotal;
    bool inlineMeridiem = false;
    if (mStyle.is12h) {
        int16_t withMeridiem = static_cast<int16_t>(digitsTotal + kMeridiemGap + meridiemText.getTextWidth());
        if (withMeridiem <= kFaceWidth - 12) {
            total = withMeridiem;
            inlineMeridiem = true;
        }
    }

    int16_t x = static_cast<int16_t>((kFaceWidth - total) / 2);

    place(hourText, x, clockY, hourWidth, clockHeight);
    x = static_cast<int16_t>(x + hourWidth);

    place(colonText, x, clockY, colonWidth, clockHeight);
    x = static_cast<int16_t>(x + colonWidth);

    place(minuteText, x, clockY, minuteWidth, clockHeight);
    x = static_cast<int16_t>(x + minuteWidth);

    if (mStyle.showSeconds) {
        place(mColon2Text, x, clockY, colon2Width, clockHeight);
        x = static_cast<int16_t>(x + colon2Width);

        place(mSecondText, x, clockY, secondWidth, clockHeight);
        x = static_cast<int16_t>(x + secondWidth);

        mColon2Text.setVisible(true);
        mColon2Text.invalidate();
        mSecondText.setVisible(true);
        mSecondText.invalidate();
    } else {
        mColon2Text.setVisible(false);
        mColon2Text.invalidate();
        mSecondText.setVisible(false);
        mSecondText.invalidate();
    }

    x = static_cast<int16_t>(x + kMeridiemGap);

    if (mStyle.is12h) {
        const int16_t mWidth = static_cast<int16_t>(meridiemText.getTextWidth());
        const int16_t meridiemDrop = mStyle.showSeconds ? kMeridiemSecondsDrop : kMeridiemDrop;
        if (inlineMeridiem) {
            place(meridiemText, x, static_cast<int16_t>(clockY + meridiemDrop),
                  mWidth, kMeridiemHeight);
        } else {
            // Tucked neatly above the top-right of the rightmost digits
            place(meridiemText, static_cast<int16_t>(kFaceWidth - mWidth - 10),
                  static_cast<int16_t>(clockY - kMeridiemHeight + 4),
                  mWidth, kMeridiemHeight);
        }
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

    if (mStyle.showSeconds) {
        Unicode::snprintf(mSecondTextBuffer, SECONDTEXT_SIZE, "%02u",
                          static_cast<unsigned>(mShown.second));
    }
}

void MainView::updateDateText()
{
    const char* dayName = kDayNames[mShown.wday % 7u];
    const char* monthName = kMonthNames[mShown.mon % 12u];
    const unsigned dayNum = static_cast<unsigned>(mShown.mday);

    char temp[32];
    if (mStyle.monthFirst) {
        // e.g. "Fri Oct 02"
        snprintf(temp, sizeof(temp), "%s %s %02u", dayName, monthName, dayNum);
    } else {
        // e.g. "Fri 02 Oct"
        snprintf(temp, sizeof(temp), "%s %02u %s", dayName, dayNum, monthName);
    }

    // Convert 8-bit ASCII string directly to TouchGFX 16-bit UnicodeChar
    for (uint16_t i = 0; i < DATE_BUFFER_SIZE; ++i) {
        mDateBuffer[i] = static_cast<touchgfx::Unicode::UnicodeChar>(temp[i]);
        if (temp[i] == '\0') {
            break;
        }
    }
    mDateBuffer[DATE_BUFFER_SIZE - 1] = 0;

    weekdayText.invalidate();
}

void MainView::setTime(const WallTime &time)
{
    if (time == mShown) {
        return;
    }

    const bool minuteAndDateSame =
        (time.hour == mShown.hour) && (time.minute == mShown.minute) &&
        (time.mday == mShown.mday) && (time.wday == mShown.wday) &&
        (time.mon == mShown.mon);

    mShown = time;

    if (minuteAndDateSame) {
        if (mStyle.showSeconds && mSecondText.isVisible()) {
            Unicode::snprintf(mSecondTextBuffer, SECONDTEXT_SIZE, "%02u",
                              static_cast<unsigned>(mShown.second));
            mSecondText.invalidate();
        }
        return;
    }

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
    setBattery(level, mBatteryEnabled);
}

void MainView::setBattery(uint8_t level, bool enabled)
{
    mBatteryLevel   = level;
    mBatteryEnabled = enabled;

    if (!mBatteryEnabled) {
        if (mBatteryContainer.isVisible()) {
            mBatteryContainer.setVisible(false);
            mBatteryContainer.invalidate();
        }
        return;
    }

    if (!mBatteryContainer.isVisible()) {
        mBatteryContainer.setVisible(true);
    }

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
