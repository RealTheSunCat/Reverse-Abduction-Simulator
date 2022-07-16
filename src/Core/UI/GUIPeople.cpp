#include "GUIPeople.h"

#include "UIHuman.h"
#include "UIButton.h"
#include "GUIStats.h"

GUIPeople::GUIPeople() : GUILayer("People renderer", false)
{

}

GUIPeople::~GUIPeople()
{

}


void GUIPeople::tick()
{
    for(auto& button : buttons)
    {
        button->tick();
    }

    for(int i = 0; i < m_people.size(); i++)
    {
        UIHuman& human = m_people[i];

        if(!human.hasGoal()) {

            // random chance to assign a new goal
            float random = rand() / float(RAND_MAX);

            if(random >= 0.995) {

                float r = 300 * sqrt(rand() / float(RAND_MAX));
                float theta = (rand() / float(RAND_MAX)) * 2 * M_PI;

                glm::vec2 goal(1100 + 440 - 96, 20 + 440 - 135);
                goal.x += r * cos(theta);
                goal.y += r * sin(theta);

                human.setGoal(goal.x, goal.y);

            }
        }

        human.tick();
    }

    // update stats
    int peopleCount = 0;
    for(UIHuman& h : m_people) {
        if(!h.isDead())
            peopleCount++;
    }

    ((GUIStats*) Outrospection::get().layerPtrs["stats"])->setPeopleCount(peopleCount);
}

void GUIPeople::draw() const
{
    for(auto& button : buttons)
    {
        button->draw();
    }

    for(auto& human : m_people)
    {
        human.draw();
    }
}


void GUIPeople::addHuman(UIHuman human)
{
    human.setScale(192, 270);
    human.setPosition(1050, 80);

    human.setGoal(1550, 520);
    m_people.emplace_back(human);

    int newHumanIndex = m_people.size() - 1;

    if(human.isBad()) {
        Util::doLater([this]() {
            for(int i = 0; i < m_people.size() - 1; i++) // exclude the new human that is bad
            {
                if(m_people[i].isDead())
                    continue;

                float random = rand() / float(RAND_MAX);
                if(random > 0.75)
                {
                    m_people[i].markForDeletion();
                }
            }
        }, 500);

        // delete bad human
        m_people[newHumanIndex].markForDeletion();

    }
}
