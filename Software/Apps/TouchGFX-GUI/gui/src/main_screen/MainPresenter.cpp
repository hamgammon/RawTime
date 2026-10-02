#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{

}

void MainPresenter::activate()
{

}

void MainPresenter::deactivate()
{

}

void MainPresenter::onTime(const WallTime &time)
{
    view.setTime(time);
}

void MainPresenter::onSteps(uint32_t steps)
{
    view.setSteps(steps);
}

void MainPresenter::onBatteryLevel(uint8_t level)
{
    view.setBatteryLevel(level);
}

void MainPresenter::onClockStyle(const ClockStyle &style)
{
    view.setClockStyle(style);
}
