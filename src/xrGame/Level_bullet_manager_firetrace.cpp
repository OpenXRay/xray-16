// Level_Bullet_Manager.cpp:	для обеспечения полета пули по траектории
//								все пули и осколки передаются сюда
//								(для просчета столкновений и их визуализации)
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Level_Bullet_Manager.h"
#include "Entity.h"
#include "xrEngine/GameMtlLib.h"
#include "Level.h"
#include "GamePersistent.h"
#include "game_cl_base.h"
#include "xrMessages.h"
#include "Include/xrRender/Kinematics.h"
#include "Actor.h"
#include "ai/stalker/ai_stalker.h"
#include "character_info.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "xrCDB/xr_collide_defs.h"
#include "xrEngine/xr_collide_form.h"
#include "Weapon.h"
#include "ik/math3d.h"
#include "Actor.h"
#include "ai/monsters/basemonster/base_monster.h"

//константы ShootFactor, определяющие
//поведение пули при столкновении с объектом
// XXX: review
#define RICOCHET_THRESHOLD 0.1
#define STUCK_THRESHOLD 0.4

//расстояния не пролетев которого пуля не трогает того кто ее пустил
extern float gCheckHitK;

// test callback функция
//  object - object for testing
// return TRUE-тестировать объект / FALSE-пропустить объект
BOOL CBulletManager::test_callback(const collide::ray_defs& rd, IGameObject* object, LPVOID params)
{
    if (!object)
        return TRUE;

    bullet_test_callback_data* pData = (bullet_test_callback_data*)params;
    SBullet* bullet = pData->pBullet;

    if ((object->ID() == bullet->parent_id) && (bullet->fly_dist < parent_ignore_distance) &&
        (!bullet->flags.ricochet_was))
        return FALSE;

    BOOL bRes = TRUE;
    CEntity* entity = smart_cast<CEntity*>(object);
    if (entity && entity->g_Alive() && (entity->ID() != bullet->parent_id))
    {
        ICollisionForm* cform = entity->GetCForm();
        if ((NULL != cform) && (cftObject == cform->Type()))
        {
            CActor* actor = smart_cast<CActor*>(entity);
            CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(entity);
            // в кого попали?
            if (actor && IsGameTypeSingle() /**/ || stalker /**/)
            {
                // попали в актера или сталкера
                Fsphere S = cform->getSphere();
                entity->XFORM().transform_tiny(S.P);
                float dist = rd.range;
                // проверим попали ли мы в описывающую сферу
                if (Fsphere::rpNone != S.intersect_full(bullet->bullet_pos, bullet->dir, dist))
                {
                    // да попали, найдем кто стрелял
                    bool play_whine = true;
                    IGameObject* initiator = Level().Objects.net_Find(bullet->parent_id);
                    if (actor)
                    {
                        // попали в актера
                        float hpf = 1.f;
                        float ahp = actor->HitProbability();
#if 1
#if 0
                        IGameObject					*weapon_object = Level().Objects.net_Find	(bullet->weapon_id);
                        if (weapon_object) {
                            CWeapon				*weapon = smart_cast<CWeapon*>(weapon_object);
                            if (weapon) {
                                float fly_dist		= bullet->fly_dist+dist;
                                float dist_factor	= _min(1.f,fly_dist/Level().BulletManager().m_fHPMaxDist);
                                ahp					= dist_factor*weapon->hit_probability() + (1.f-dist_factor)*1.f;
                            }
                        }
#else
                        float game_difficulty_hit_probability = actor->HitProbability();
                        CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(initiator);
                        if (stalker)
                            hpf = stalker->SpecificCharacter().hit_probability_factor();

                        float dist_factor = 1.f;
                        IGameObject* weapon_object = Level().Objects.net_Find(bullet->weapon_id);
                        if (weapon_object)
                        {
                            CWeapon* weapon = smart_cast<CWeapon*>(weapon_object);
                            if (weapon)
                            {
                                game_difficulty_hit_probability = weapon->hit_probability();
                                float fly_dist = bullet->fly_dist + dist;
                                dist_factor = _min(1.f, fly_dist / Level().BulletManager().m_fHPMaxDist);
                            }
                        }

                        ahp = dist_factor * game_difficulty_hit_probability + (1.f - dist_factor) * 1.f;
#endif
#else
                        CAI_Stalker* i_stalker = smart_cast<CAI_Stalker*>(initiator);
                        // если стрелял сталкер, учитываем - hit_probability_factor сталкерa иначе - 1.0
                        if (i_stalker)
                        {
                            hpf = i_stalker->SpecificCharacter().hit_probability_factor();
                            float fly_dist = bullet->fly_dist + dist;
                            float dist_factor = _min(1.f, fly_dist / Level().BulletManager().m_fHPMaxDist);
                            ahp = dist_factor * actor->HitProbability() + (1.f - dist_factor) * 1.f;
                        }
#endif
                        if (Random.randF(0.f, 1.f) > (ahp * hpf))
                        {
                            bRes = FALSE; // don't hit actor
                            play_whine = true; // play whine sound
                        }
                        else
                        {
                            // real test actor CFORM
                            Level().BulletManager().m_rq_results.r_clear();

                            if (cform->_RayQuery(rd, Level().BulletManager().m_rq_results))
                            {
                                bRes = TRUE; // hit actor
                                play_whine = false; // don't play whine sound
                            }
                            else
                            {
                                bRes = FALSE; // don't hit actor
                                play_whine = true; // play whine sound
                            }
                        }
                    }
                    // play whine sound
                    if (play_whine)
                    {
                        Fvector pt;
                        pt.mad(bullet->bullet_pos, bullet->dir, dist);
                        Level().BulletManager().PlayWhineSound(bullet, initiator, pt);
                    }
                }
                else
                {
                    // don't test this object again (return FALSE)
                    bRes = FALSE;
                }
            }
        }
    }

    return bRes;
}

// callback функция
//	result.O;		// 0-static else IGameObject*
//	result.range;	// range from start to element
//	result.element;	// if (O) "num tri" else "num bone"
//	params;			// user defined abstract data
//	Device.Statistic.TEST0.End();
// return TRUE-продолжить трассировку / FALSE-закончить трассировку

void CBulletManager::FireShotmark(SBullet* bullet, const Fvector& vDir, const Fvector& vEnd, collide::rq_result& R,
    u16 target_material, const Fvector& vNormal, bool ShowMark)
{
    SGameMtlPair* mtl_pair = GMLib.GetMaterialPairByIndices(bullet->bullet_material_idx, target_material);
    Fvector particle_dir = vNormal;

    if (R.O)
    {
        particle_dir = vDir;
        particle_dir.invert();

        // XXX: review
        // на текущем актере отметок не ставим
        if (Level().CurrentEntity() && Level().CurrentEntity()->ID() == R.O->ID())
            return;

        if (mtl_pair && !mtl_pair->CollideMarks->empty() && ShowMark)
        {
            //добавить отметку на материале
            Fvector p;
            p.mad(bullet->bullet_pos, bullet->dir, R.range - 0.01f);
            if (!GEnv.isDedicatedServer)
                GEnv.Render->add_SkeletonWallmark(&R.O->XFORM(),
                                                  PKinematics(R.O->Visual()),
                                                  &*mtl_pair->CollideMarks,
                                                  p,
                                                  bullet->dir,
                                                  bullet->wallmark_size);
        }
    }
    else
    {
        //вычислить нормаль к пораженной поверхности
        Fvector* pVerts = Level().ObjectSpace.GetStaticVerts();
        CDB::TRI* pTri = Level().ObjectSpace.GetStaticTris() + R.element;

        if (mtl_pair && !mtl_pair->CollideMarks->empty() && ShowMark)
        {
            //добавить отметку на материале
            GEnv.Render->add_StaticWallmark(&*mtl_pair->CollideMarks, vEnd, bullet->wallmark_size, pTri, pVerts);
        }
    }

    ref_sound* pSound = (!mtl_pair || mtl_pair->CollideSounds.empty()) ?
        NULL :
        &mtl_pair->CollideSounds[::Random.randI(0, mtl_pair->CollideSounds.size())];

    //проиграть звук
    if (pSound && ShowMark)
    {
        IGameObject* O = Level().Objects.net_Find(bullet->parent_id);
        bullet->m_mtl_snd = *pSound;
        bullet->m_mtl_snd.play_at_pos(O, vEnd, 0);
    }

    LPCSTR ps_name = (!mtl_pair || mtl_pair->CollideParticles.empty()) ?
        NULL :
        *mtl_pair->CollideParticles[::Random.randI(0, mtl_pair->CollideParticles.size())];

    SGameMtl* tgt_mtl = GMLib.GetMaterialByIdx(target_material);
    BOOL bStatic = !tgt_mtl->Flags.test(SGameMtl::flDynamic);

    if ((ps_name && ShowMark) || (bullet->flags.explosive && bStatic))
    {
        VERIFY2((particle_dir.x * particle_dir.x + particle_dir.y * particle_dir.y + particle_dir.z * particle_dir.z) >
                flt_zero,
            make_string("[%f][%f][%f]", VPUSH(particle_dir)));
        Fmatrix pos;
        pos.k.normalize(particle_dir);
        Fvector::generate_orthonormal_basis(pos.k, pos.j, pos.i);
        pos.c.set(vEnd);
        if (ps_name && ShowMark)
        {
            //отыграть партиклы попадания в материал
            CParticlesObject* ps = CParticlesObject::Create(ps_name, TRUE);

            ps->UpdateParent(pos, zero_vel);
            GamePersistent().ps_needtoplay.push_back(ps);
        }

        if (bullet->flags.explosive && bStatic)
        {
            PlayExplodePS(pos);
        }
    }
}

void CBulletManager::StaticObjectHit(CBulletManager::_event& E)
{
    //	Fvector hit_normal;
    FireShotmark(&E.bullet, E.bullet.dir, E.point, E.R, E.tgt_material, E.normal);
    //	ObjectHit	(&E.bullet,					E.point, E.R, E.tgt_material, hit_normal);
}

static bool g_clear = false;
void CBulletManager::DynamicObjectHit(CBulletManager::_event& E)
{
    //только для динамических объектов
    VERIFY(E.R.O);

    if (CEntity* entity = smart_cast<CEntity*>(E.R.O))
    {
        if (!entity->in_solid_state())
        {
            return;
        }
    }

    if (g_clear)
        E.Repeated = false;
    if (GameID() == eGameIDSingle)
        E.Repeated = false;
    bool NeedShootmark = true; //! E.Repeated;

    if (smart_cast<CActor*>(E.R.O))
    {
        game_PlayerState* ps = Game().GetPlayerByGameID(E.R.O->ID());
        if (ps && ps->testFlag(GAME_PLAYER_FLAG_INVINCIBLE))
        {
            NeedShootmark = false;
        };
    }
    else if (CBaseMonster* monster = smart_cast<CBaseMonster*>(E.R.O))
    {
        NeedShootmark = monster->need_shotmark();
    }

    //визуальное обозначение попадание на объекте
    //	Fvector			hit_normal;
    FireShotmark(&E.bullet, E.bullet.dir, E.point, E.R, E.tgt_material, E.normal, NeedShootmark);

    Fvector original_dir = E.bullet.dir;
    // ObjectHit(&E.bullet, E.end_point, E.R, E.tgt_material, hit_normal);

    SBullet_Hit hit_param = E.hit_result;

    // object-space
    //вычислить координаты попадания
    Fvector p_in_object_space, position_in_bone_space;
    Fmatrix m_inv;
    m_inv.invert(E.R.O->XFORM());
    m_inv.transform_tiny(p_in_object_space, E.point);

    // bone-space
    IKinematics* V = smart_cast<IKinematics*>(E.R.O->Visual());

    if (V)
    {
        VERIFY3(V->LL_GetBoneVisible(u16(E.R.element)), *E.R.O->cNameVisual(), V->LL_BoneName_dbg(u16(E.R.element)));
        Fmatrix& m_bone = (V->LL_GetBoneInstance(u16(E.R.element))).mTransform;
        Fmatrix m_inv_bone;
        m_inv_bone.invert(m_bone);
        m_inv_bone.transform_tiny(position_in_bone_space, p_in_object_space);
    }
    else
    {
        position_in_bone_space.set(p_in_object_space);
    }

    //отправить хит пораженному объекту
    if (E.bullet.flags.allow_sendhit && !E.Repeated)
    {
        //-------------------------------------------------
        bool AddStatistic = false;
        if (GameID() != eGameIDSingle && E.bullet.flags.allow_sendhit && smart_cast<CActor*>(E.R.O) &&
            Game().m_WeaponUsageStatistic->CollectData())
        {
            CActor* pActor = smart_cast<CActor*>(E.R.O);
            if (pActor) // && pActor->g_Alive())
            {
                Game().m_WeaponUsageStatistic->OnBullet_Hit(&E.bullet, E.R.O->ID(), (s16)E.R.element, E.point);
                AddStatistic = true;
            };
        };

        SHit Hit = SHit(hit_param.power, original_dir, NULL, u16(E.R.element), position_in_bone_space,
            hit_param.impulse, E.bullet.hit_type, E.bullet.armor_piercing, E.bullet.flags.aim_bullet);

        Hit.GenHeader(u16((AddStatistic) ? GE_HIT_STATISTIC : GE_HIT) & 0xffff, E.R.O->ID());
        Hit.whoID = E.bullet.parent_id;
        Hit.weaponID = E.bullet.weapon_id;
        Hit.BulletID = E.bullet.m_dwID;

        NET_Packet np;
        Hit.Write_Packet(np);

        //		Msg("Hit sended: %d[%d,%d]", Hit.whoID, Hit.weaponID, Hit.BulletID);
        CGameObject::u_EventSend(np);
    }
}

#ifdef DEBUG
FvectorVec g_hit[3];
#endif

extern void random_dir(Fvector& tgt_dir, const Fvector& src_dir, float dispersion);

bool CBulletManager::ObjectHit(SBullet_Hit* hit_res, SBullet* bullet, const Fvector& end_point, collide::rq_result& R,
    u16 target_material, Fvector& hit_normal)
{
    //----------- normal - start
    if (R.O)
    {
        //вернуть нормаль по которой играть партиклы
        CCF_Skeleton* skeleton = smart_cast<CCF_Skeleton*>(R.O->GetCForm());
        if (skeleton)
        {
            Fvector e_center;
            hit_normal.set(0, 0, 0);
            if (skeleton->_ElementCenter((u16)R.element, e_center))
                hit_normal.sub(end_point, e_center);
            float len = hit_normal.square_magnitude();
            if (!fis_zero(len))
                hit_normal.div(_sqrt(len));
            else
                hit_normal.invert(bullet->dir);
        }
    }
    else
    {
        //вычислить нормаль к поверхности
        Fvector* pVerts = Level().ObjectSpace.GetStaticVerts();
        CDB::TRI* pTri = Level().ObjectSpace.GetStaticTris() + R.element;
        hit_normal.mknormal(pVerts[pTri->verts[0]], pVerts[pTri->verts[1]], pVerts[pTri->verts[2]]);
        if (bullet->density_mode)
        {
            Fvector new_pos;
            new_pos.mad(bullet->bullet_pos, bullet->dir, R.range);
            float l = bullet->begin_density.distance_to(new_pos);
            float shootFactor = l * bullet->density;
            bullet->speed -= shootFactor;
            if (bullet->speed < 0)
                bullet->speed = 0;
        }
        if (DOT(hit_normal, bullet->dir) < 0)
        {
            if (bullet->density_mode)
            {
                //				Log("WARNING: Material in material found while bullet tracing. Incorrect behaviour of
                //shooting
                // is possible.");
            }
            bullet->density_mode = true;
            SGameMtl* mtl = GMLib.GetMaterialByIdx(target_material);
            bullet->density = mtl->fDensityFactor;
            bullet->begin_density.mad(bullet->bullet_pos, bullet->dir, R.range);
        }
        else
        {
            bullet->density_mode = false;
        }
    }
    //----------- normal - end
    float old_speed = bullet->speed;

    //коэффициент уменьшение силы с падением скорости
    float speed_factor = bullet->speed / bullet->max_speed;
    //получить силу хита выстрела с учетом патрона
    *hit_res = bullet->hit_param; // default param

    hit_res->power = bullet->hit_param.power * speed_factor;

    //(Если = 0, то пуля либо рикошетит(если контакт идёт по касательной), либо застряёт в текущем
    //объекте, если больше 0, то пуля прошивает объект)

    SGameMtl* mtl = GMLib.GetMaterialByIdx(target_material);
    float mtl_ap = mtl->fShootFactor;
    float shoot_factor = 0.0f; // default >> пуля НЕ пробила материал!
    float ap = bullet->armor_piercing;

    if (ap > EPS && ap >= mtl_ap)
    {
        //пуля пробила материал
        shoot_factor = ((ap - mtl_ap) / ap);
    }

    hit_res->impulse = 0.0f;
    float speed_scale = 0.0f;

#ifdef DEBUG
    // Fvector dbg_bullet_pos;
    // dbg_bullet_pos.mad(bullet->bullet_pos,bullet->dir,R.range);
    int bullet_state = 0;
#endif

    if (fsimilar(mtl_ap, 0.0f)) //Если материал полностью простреливаемый (кусты)
    {
#ifdef DEBUG
        bullet_state = 2;
#endif
        return true;
    }

    if (bullet->flags.magnetic_beam && (shoot_factor > EPS))
    {
#ifdef DEBUG
        bullet_state = 2;
#endif
        // air resistance of magnetic_beam bullet is armor resistance too
        bullet->armor_piercing -= mtl_ap * bullet->air_resistance;
        return true;
    }

    //рикошет
    Fvector new_dir;
    new_dir.reflect(bullet->dir, hit_normal);
    Fvector tgt_dir;
    random_dir(tgt_dir, new_dir, deg2rad(10.0f));
    float ricoshet_factor = bullet->dir.dotproduct(tgt_dir);

    float f = Random.randF(0.5f, 0.8f); //(0.5f,1.f);
    if ((f < ricoshet_factor) && !mtl->Flags.test(SGameMtl::flNoRicoshet) && bullet->flags.allow_ricochet)
    {
        // уменьшение скорости полета в зависимости от угла падения пули (чем прямее угол, тем больше потеря)
        bullet->flags.allow_ricochet = 0;
        float scale = 1.0f - _abs(bullet->dir.dotproduct(hit_normal)) * m_fCollisionEnergyMin;
        clamp(scale, 0.0f, m_fCollisionEnergyMax);
        speed_scale = scale;

        // вычисление рикошета, делается немного фейком, т.к. пуля остается в точке столкновения
        // и сразу выходит из RayQuery()
        bullet->dir.set(tgt_dir);
        bullet->bullet_pos = end_point;
        bullet->flags.ricochet_was = 1;

#ifdef DEBUG
        bullet_state = 0;
#endif
    }
    else if (shoot_factor < EPS)
    {
        //застрявание пули в материале
        speed_scale = 0.0f;
#ifdef DEBUG
        bullet_state = 1;
#endif
    }
    else
    {
        //пробивание материала
        speed_scale = shoot_factor; // mtl->fShootFactor;

        bullet->bullet_pos.mad(bullet->bullet_pos, bullet->dir, EPS); // fake
        //ввести коэффициент случайности при простреливании
        Fvector rand_normal;
        rand_normal.random_dir(bullet->dir, deg2rad(2.0f), Random);
        bullet->dir.set(rand_normal);
#ifdef DEBUG
        bullet_state = 2;
#endif
    }

    //уменьшить скорость в зависимости от простреливаемости
    bullet->speed *= speed_scale;
    //сколько энергии в процентах потеряла пуля при столкновении
    float energy_lost = 1.0f - bullet->speed / old_speed;
    //импульс переданный объекту равен прямопропорционален потерянной энергии
    hit_res->impulse = bullet->hit_param.impulse * speed_factor * energy_lost;

#ifdef DEBUG
    extern BOOL g_bDrawBulletHit;
    if (g_bDrawBulletHit)
    {
        //		g_hit[bullet_state].push_back(dbg_bullet_pos);
        g_hit[bullet_state].push_back(end_point);
    }
#endif

    return true;
}
