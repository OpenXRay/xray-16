#pragma once

template <typename _Object>
CTelekinesis<_Object>::CTelekinesis()
{
	active = false;
}

template <typename _Object>
CTelekinesis<_Object>::~CTelekinesis()
{

}

template <typename _Object>
void CTelekinesis<_Object>::InitExtern(_Object *pO, float s, float h, u32 keep_time)
{
	control_object	= pO;
	strength		= s;
	height			= h;
	max_time_keep	= keep_time;
}

template <typename _Object>
void CTelekinesis<_Object>::Activate()
{
	if (active)		return;
	VERIFY(objects.empty());

	active = true;

	// получить список объектов
	m_nearest.clear_not_free			();
	Level().ObjectSpace.GetNearest		(m_nearest,control_object->Position(),10.f); 
	//xr_vector<CObject*> &m_nearest		= Level().ObjectSpace.q_nearest; 

	// все объекты внести в список 
	for (u32 i = 0; i < m_nearest.size(); i++) {
		
		CGameObject *obj = smart_cast<CGameObject *>(m_nearest[i]);
		if (!obj || !obj->m_pPhysicsShell) continue;
		
		// отключить гравитацию
		obj->m_pPhysicsShell->set_ApplyByGravity(FALSE);
		
		CTelekineticObject tele_object;

		tele_object.init(obj,height);
		// добавить объект
		objects.push_back(tele_object);
	}

	if (!objects.empty()) CPHUpdateObject::Activate();
}

template <typename _Object>
void CTelekinesis<_Object>::Deactivate()
{
	active			= false;

	for (u32 i = 0; i < objects.size(); i++) {
		objects[i].release();
	}

	objects.clear	();

	CPHUpdateObject::Deactivate();
}



template <typename _Object>
void CTelekinesis<_Object>::Throw(const Fvector &target)
{
	if (!active) return;

	for (u32 i = 0; i < objects.size(); i++) {
		objects[i].fire(target);
	}

	Deactivate();
}


template <typename _Object>
void CTelekinesis<_Object>::UpdateSched()
{
	if (!active) return;
	
	// обновить состояние объектов
	for (u32 i = 0; i < objects.size(); i++) {
		CTelekineticObject *cur_obj = &objects[i]; 
		switch (cur_obj->get_state()) {
		case TS_Raise: 
				if (cur_obj->check_height()) cur_obj->prepare_keep();// начать удержание предмета
			break;
		case TS_Keep:
			if (cur_obj->time_keep_elapsed()) {
				cur_obj->release();
				
				// удалить объект из массива
				if (objects.size() > 1) {
					if (i != (objects.size()-1)) objects[i] = objects.back();
					objects.pop_back();
				} else {
					objects.clear();
					active = false;
				}
			}
			break;
		case TS_None: continue; 
		}
	}
}

template <typename _Object>
void CTelekinesis<_Object>::PhDataUpdate(dReal step)
{
	if (!active) return;

	for (u32 i = 0; i < objects.size(); i++) {
		switch (objects[i].get_state()) {
		case TS_Raise:	objects[i].raise(strength * step); break;
		case TS_Keep:	objects[i].keep(); break;
		case TS_None:	break;
		}
	}
}


template <typename _Object>
void  CTelekinesis<_Object>::PhTune(dReal step)
{
	for (u32 i = 0; i < objects.size(); i++) {
		switch (objects[i].get_state()) {
		case TS_Raise:	
		case TS_Keep:	objects[i].get_object()->m_pPhysicsShell->Enable();
		case TS_None:	break;
		}
	}
}


