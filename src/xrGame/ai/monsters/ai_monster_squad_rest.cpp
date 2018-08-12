#include "StdAfx.h"
#include "ai_monster_squad.h"
#include "Entity.h"
#include "xrAICore/Navigation/ai_object_location.h"

void CMonsterSquad::ProcessIdle()
{
    m_temp_entities.clear();
    VERIFY(leader && !leader->getDestroy());

    // Выделить элементы с общими врагами и состянием атаки
    for (auto it_goal = m_goals.begin(); it_goal != m_goals.end(); it_goal++)
    {
        SMemberGoal goal = it_goal->second;
        if ((goal.type == MG_Rest) || (goal.type == MG_WalkGraph))
        {
            m_temp_entities.push_back(it_goal->first);
        }
    }

    Idle_AssignAction(m_temp_entities);
}

struct CPredicateSideSort
{
    Fvector target;

    CPredicateSideSort(Fvector pos) { target = pos; }
    bool operator()(CEntity* e1, CEntity* e2)
    {
        return (e1->Position().distance_to_sqr(target) > e2->Position().distance_to_sqr(target));
    }
};

#define CENTER_CIRCLE_DIST 20
#define CIRCLE_RADIUS_MIN 10
#define CIRCLE_RADIUS_MAX 15

void CMonsterSquad::Idle_AssignAction(ENTITY_VEC& members)
{
    // получить цель лидера
    SMemberGoal& goal = GetGoal(leader);

    if (goal.type == MG_WalkGraph)
    {
        front.clear();
        back.clear();
        left.clear();
        right.clear();

        for (auto IT = members.begin(); IT != members.end(); IT++)
        {
            if ((*IT) == leader)
                continue;

            front.push_back(*IT);
            back.push_back(*IT);
            left.push_back(*IT);
            right.push_back(*IT);
        }

        Fvector front_pos;
        Fvector back_pos;
        Fvector left_pos;
        Fvector right_pos;

        Fvector dir = leader->Direction();
        front_pos.mad(leader->Position(), dir, CENTER_CIRCLE_DIST);
        std::sort(front.begin(), front.end(), CPredicateSideSort(front_pos));

        dir.invert();
        back_pos.mad(leader->Position(), dir, CENTER_CIRCLE_DIST);
        std::sort(back.begin(), back.end(), CPredicateSideSort(back_pos));

        dir = leader->XFORM().i;
        right_pos.mad(leader->Position(), dir, CENTER_CIRCLE_DIST);
        std::sort(right.begin(), right.end(), CPredicateSideSort(right_pos));

        dir.invert();
        left_pos.mad(leader->Position(), dir, CENTER_CIRCLE_DIST);
        std::sort(left.begin(), left.end(), CPredicateSideSort(left_pos));

        SSquadCommand command;
        command.type = SC_FOLLOW;
        command.entity = leader;
        command.direction = leader->Direction();

        u8 cur_type = 0;
        while (!front.empty())
        {
            float random_r;
            Fvector random_dir;

            random_dir.random_dir();
            random_r = Random.randF(CIRCLE_RADIUS_MIN, CIRCLE_RADIUS_MAX);

            const CEntity* entity = 0;
            switch (cur_type)
            {
            case 0: // front
                entity = front.back();
                front.pop_back();
                for (u32 i = 0; i < back.size(); i++)
                    if (back[i] == entity)
                    {
                        back[i] = back.back();
                        back.pop_back();
                        break;
                    }
                for (u32 i = 0; i < right.size(); i++)
                    if (right[i] == entity)
                    {
                        right[i] = right.back();
                        right.pop_back();
                        break;
                    }
                for (u32 i = 0; i < left.size(); i++)
                    if (left[i] == entity)
                    {
                        left[i] = left.back();
                        left.pop_back();
                        break;
                    }
                command.position.mad(front_pos, random_dir, random_r);
                break;
            case 1: // back
                entity = back.back();
                back.pop_back();
                for (u32 i = 0; i < front.size(); i++)
                    if (front[i] == entity)
                    {
                        front[i] = front.back();
                        front.pop_back();
                        break;
                    }
                for (u32 i = 0; i < right.size(); i++)
                    if (right[i] == entity)
                    {
                        right[i] = right.back();
                        right.pop_back();
                        break;
                    }
                for (u32 i = 0; i < left.size(); i++)
                    if (left[i] == entity)
                    {
                        left[i] = left.back();
                        left.pop_back();
                        break;
                    }
                command.position.mad(back_pos, random_dir, random_r);
                break;
            case 2: // left
                entity = left.back();
                left.pop_back();
                for (u32 i = 0; i < front.size(); i++)
                    if (front[i] == entity)
                    {
                        front[i] = front.back();
                        front.pop_back();
                        break;
                    }
                for (u32 i = 0; i < right.size(); i++)
                    if (right[i] == entity)
                    {
                        right[i] = right.back();
                        right.pop_back();
                        break;
                    }
                for (u32 i = 0; i < back.size(); i++)
                    if (back[i] == entity)
                    {
                        back[i] = back.back();
                        back.pop_back();
                        break;
                    }
                command.position.mad(left_pos, random_dir, random_r);
                break;
            case 3: // right
                entity = right.back();
                right.pop_back();
                for (u32 i = 0; i < front.size(); i++)
                    if (front[i] == entity)
                    {
                        front[i] = front.back();
                        front.pop_back();
                        break;
                    }
                for (u32 i = 0; i < left.size(); i++)
                    if (left[i] == entity)
                    {
                        left[i] = left.back();
                        left.pop_back();
                        break;
                    }
                for (u32 i = 0; i < back.size(); i++)
                    if (back[i] == entity)
                    {
                        back[i] = back.back();
                        back.pop_back();
                        break;
                    }
                command.position.mad(right_pos, random_dir, random_r);
                break;
            default: NODEFAULT;
            }

            cur_type++;
            if (cur_type > 3)
                cur_type = 0;

            UpdateCommand(entity, command);
        }
    }
    else if (goal.type == MG_Rest)
    {
        // пересчитать положение в команде в соответствие с целью лидера
        for (auto it = members.begin(); it != members.end(); it++)
        {
            if ((*it) == leader)
                continue;

            SSquadCommand command;
            command.type = SC_REST;
            command.position = leader->Position();
            command.node = leader->ai_location().level_vertex_id();
            command.entity = 0;

            UpdateCommand(*it, command);
        }
    }
}
