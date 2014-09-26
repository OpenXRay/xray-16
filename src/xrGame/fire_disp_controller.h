#ifndef FIRE_DISP_CONTROLLER
#define FIRE_DISP_CONTROLLER


class CFireDispertionController
{
private:
	float start_disp;
	float end_disp;
	float start_time;
	float current_disp;
	static float const default_inertion;
public:
	CFireDispertionController		();
	void	SetDispertion			(float const new_disp);
	float	GetCurrentDispertion	() const;
	void	Update					();
}; //class CFireDispertionController


#endif //#ifndef FIRE_DISP_CONTROLLER