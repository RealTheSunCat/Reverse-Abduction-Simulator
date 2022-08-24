#include "GUIStats.h"

#include <sstream>

#include "UIButton.h"
#include "GUIPeople.h"
#include "GUICharacterMaker.h"
#include "GUIPostGame.h"

GUIStats::GUIStats() : GUILayer("Stats", false), m_timerDisplay("00:00", Resource(), UITransform(1370, 940, 100, 100)),
                        m_bossIsBack("Boss is\nback in...", Resource(), UITransform(1080, 900, 100, 100)),
                        m_timerBlurTop("timerBlurTop", simpleTexture({"ObjectData/UI/", "timerTextBlurTop"}, GL_LINEAR), UITransform(0, 0, 1920, 1080)),
                        m_peopleCount("x 0", Resource(), UITransform(1000, 680, 100, 100)),
                        m_peopleIcon("peopleIcon", simpleTexture({"ObjectData/", "person"}, GL_LINEAR), UITransform(930, 680, 66, 80)),
                        m_planetCount("x 1", Resource(), UITransform(1030, 800, 100, 100)),
                        m_planetIcon("planetIcon", simpleTexture({"ObjectData/", "planet"}, GL_LINEAR), UITransform(930, 800, 100, 100)),
                        m_goal("person goal", simpleTexture({"ObjectData/", "goal50"}, GL_LINEAR), UITransform(700, 950, 333, 91))
{
    m_timerDisplay.textShadow = true;
    m_timerDisplay.textSize = 3.5;
    m_timerDisplay.textColor = Color(0.9216, 0.451, 0.6627);

    m_bossIsBack.textSize = 1.3;
    m_bossIsBack.textColor = Color(0.9216, 0.451, 0.6627);
    m_bossIsBack.textShadow = true;

    m_peopleCount.textSize = 1.5;
    m_peopleCount.textColor = Color(0.9843, 0.9490, 0.8039);

    m_planetCount.textSize = 1.5;
    m_planetCount.textColor = Color(0.9843, 0.9490, 0.8039);
}

GUIStats::~GUIStats()
{

}

void GUIStats::setTimer(time_t millis)
{
    m_timer.setDuration(millis);
    m_timer.start();
}

void GUIStats::setPeopleCount(int count)
{
    using namespace std;
    stringstream ss;
    ss << 'x' << setfill('0') << setw(2) << count;

    m_peopleCount.text = ss.str();
}

void GUIStats::tick()
{
    m_timer.tick();

    m_timerBlurTop.tick();
    m_bossIsBack.tick();
    m_timerDisplay.tick();
    m_peopleCount.tick();
    m_peopleIcon.tick();
    m_planetCount.tick();
    m_planetIcon.tick();
    m_goal.tick();

    using namespace std;
    stringstream ss;
    ss << setfill('0') << setw(2) << m_timer.getMinutes() << ":" << setw(2) << m_timer.getSeconds();

    m_timerDisplay.text = ss.str();

    if(m_timer.ended()) {
        m_timer.start();
        m_timer.pause();

        auto& o = Outrospection::get();
        ((GUICharacterMaker*)o.layerPtrs["characterMaker"])->moveOutOfTheWay();
        o.audioManager.play("timesUp");

        Util::doLater([&o] () {
            std::cout << "Time's up!" << std::endl;

            ((GUIPostGame*)o.layerPtrs["postGame"])->start(((GUIPeople*)o.layerPtrs["people"])->humanCount() >= 50);
        }, 4000);
    }
}

void GUIStats::draw() const
{
    m_timerBlurTop.draw();
    m_timerDisplay.draw();
    m_bossIsBack.draw();
    m_peopleCount.draw();
    m_peopleIcon.draw();
    m_planetCount.draw();
    m_planetIcon.draw();
    m_goal.draw();
}
