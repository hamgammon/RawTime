#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>

/**
 * @class MainView
 * @brief The RawTime face: a clean digital clockface with date and battery indicator.
 *
 * The time is centred in the middle of the screen (y = 88..135).
 * The date is displayed underneath the time (y = 146..188).
 * The battery indicator is placed at the bottom centre of the screen (y = 206).
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /**
     * @brief Set the clock and the date parts, if the reading changed.
     *
     * Called when the service reports a new minute, and when the face is built
     * or resumed. Nothing here is sampled per frame.
     */
    void setTime(const WallTime &time);

    /** @brief The day's step count, preserved if needed. */
    void setSteps(uint32_t steps);

    /** @brief Update the battery indicator level (0..100%). */
    void setBatteryLevel(uint8_t level);

    /**
     * @brief Adopt the clock format and the date order the watch is set to.
     */
    void setClockStyle(const ClockStyle &style);

private:
    /**
     * @brief Centre the hour, colon, minute and meridiem as one group in the screen middle.
     */
    void layoutClock();

    /**
     * @brief Layout the date underneath the time according to user's month-first setting.
     */
    void layoutDate();

    /** Move a text area, repainting what it leaves as well as where it lands. */
    static void place(touchgfx::TextArea &area, int16_t x, int16_t y,
                      int16_t width, int16_t height);

    /** Rewrite the clock's two number buffers from the reading on screen. */
    void updateClockText();

    /** Rewrite the date parts from the reading on screen. */
    void updateDateText();

    /// Vertical placement: time centred in the middle of 240x240 screen.
    static const int16_t kClockY = 88;
    static const int16_t kClockHeight = 47;

    /// The meridiem sits on the clock's baseline.
    static const int16_t kMeridiemDrop = 19;
    static const int16_t kMeridiemHeight = 22;
    static const int16_t kMeridiemGap = 4;

    /// Date placement underneath the time.
    static const int16_t kWeekdayY = 144;
    static const int16_t kWeekdayHeight = 20;
    static const int16_t kDateY = 168;
    static const int16_t kMonthHeight = 24;
    static const int16_t kDateNumHeight = 20;

    WallTime   mShown;  ///< Reading currently on the display
    ClockStyle mStyle;  ///< Settings the clock and date are drawn in

    // Battery indicator at bottom centre
    touchgfx::Container mBatteryContainer;
    touchgfx::Box       mBatteryBody;
    touchgfx::Box       mBatteryInner;
    touchgfx::Box       mBatteryNub;
    touchgfx::Box       mBatterySegments[4];
    uint8_t             mBatteryLevel { 100 };
};

#endif // MAINVIEW_HPP
