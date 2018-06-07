#ifndef OGSE_CONFIG_H
#define OGSE_CONFIG_H

#include "configurator_defines.h"
// эффекты со включением через консоль
// Саншафты								r2_sunshafts [qt_off/qt_low/qt_medium/qt_high/qt_extreme]
// Объемные саншафты					r2_sunshafts_mode volumetric
// Плоские саншафты аля Crysis			r2_sunshafts_mode screen_space
// Мягкая вода							r2_soft_water [on/off]
// Мягкие партиклы						r2_soft_particles [on/off]
// Мягкие тени							r2_soft_shadows [qt_off/qt_low/qt_medium/qt_high/qt_extreme]
// SSDO									r2_ssao_mode ssdo
// HBAO									r2_ssao_mode hbao
// Steep Parallax						r2_steep_parallax [qt_off/qt_low/qt_medium/qt_high/qt_extreme]
// Dynamic Depth Of Field				r2_dof [qt_off/qt_low/qt_medium/qt_high/qt_extreme]
// Motion Blur							r2_mblur [on/off]
// Antialiasing							r2_aa [qt_off/qt_low/qt_medium/qt_high/qt_extreme]
// FXAA Antialiasing					r2_aa_mode fxaa
// SMAA Antialiasing					r2_aa_mode smaa
// Детальный бамп						r2_detail_bump [on/off]

// PARAMETERS 
// Используйте параметры для настройки соответствующего эффекта
/////////////////////////////////////////////////////////////
// Плоские саншафты
	#define SUNSHAFTS_QUALITY 4
	#define SS_DUST
	#define SS_INTENSITY float(1.0)			// яркость лучей, работает совместно с настройками из конфига
	#define SS_BLEND_FACTOR float(0.8)		// фактор смешивания с рассеянным светом. Чем меньше значение, тем меньше "засветка" от эффекта, но теряется "объемность"
	#define SS_LENGTH float(1.0)			// длина лучей. Чем больше длина, тем меньше производительность
	#define SS_DUST_SPEED float(0.4)		// скорость частиц пыли 
	#define SS_DUST_INTENSITY float(2.0)	// яркость пылинок
	#define SS_DUST_DENSITY float(1.0)		// плотность частиц пыли 
	#define SS_DUST_SIZE float(0.7)			// размер пылинок

/////////////////////////////////////////////////////////////
// Screen-Space Directional Occlusion
	#define SSDO_RADIUS float(0.04)				// радиус семплирования в игровых метрах. Чем выше, тем более заметен эффект, но тем больше нежелательных наложений теней
	#define SSDO_GRASS_TUNING float(1.0)		// коррекция затенения травы. Чем больше, тем меньше затеняется.
	#define SSDO_DISCARD_THRESHOLD float(1.0)	// максимальная разница в глубине пикселей, при которой пиксель еще учитывается в расчете затенения. Уменьшение убирает "шлейф" в некоторых случаях.
	#define SSDO_COLOR_BLEEDING float(15.0)		// сила цвета семплов. Дает более цветные тени, но уменьшает интенсивность эффекта в целом. Для компенсации увеличивайте SSDO_BLEND_FACTOR.
	
/////////////////////////////////////////////////////////////
// Horizon-Based Ambient Occlusion	
	#define HBAO_NUM_STEPS int(3)			// Добавочное количество шагов при поиске горизонта. Улучшает качество, ухудшает производительность
	#define HBAO_RADIUS float(2.0)			// Радиус поиска окклюдера.
	#define HBAO_STEP_SIZE float(4)			// Шаг семплирования. Меньшие значения приводят к более четким теням
	#define HBAO_ANGLE_BIAS float(0.5)		// Угол в радианах для компенсации самозатенения плохо тесселированной геометрии. Увеличивать, если на  ребрах полигонов заметны тени.
	#define HBAO_THRESHOLD float(0.3)		// Порог срабатывания эффекта. Чем меньше, тем более мелкие детали затеняются.
	#define HBAO_GRASS_TUNING float(2.0)	// коррекция затенения травы. Чем больше, тем меньше затеняется.

/////////////////////////////////////////////////////////////
// improved parallax occlusion mapping
	#define POM_PARALLAX_OFFSET float(0.02)			// смещение. Чем больше, тем дальше будут выступать кирпичи.
	
/////////////////////////////////////////////////////////////
// depth of field
	//common
	#define DDOF_SKY_DIST float(10000)		// расстояние до неба. Менять, только если есть артефакты при направлении фокуса на небо.
	#define DDOF_FOCUS_CIRCLE float(10)		// диаметр области расчета усредненного фокуса, в пикселях. Увеличение до некоторого предела дает более плавный переход при смене фокуса.
	// blur
//	#define DDOF_DEPTHBLUR 					// усреднение глубины пикселей. Дает более ровный переход заблюренных областей в незаблюренные. Влияет на производительность.
	#define DDOF_PENTAGON					// включение пентагональной формы бокэ. При включении этой опции желательно ставить DDOF_SAMPLES = 5, DDOF_RINGS - 15-20 (ogse_dof.h)
		#define DDOF_FEATHER float(0.4) 	// размер бокэ
		#define DDOF_FSTOP float(-0.2)		// размер незатененной области на экране. Можно ставить отрицательные значения. При значениях более 0,7-DDOF_VIGNOUT эффект есть только в углах экрана. При значениях более 0,9-DDOF_VIGNOUT эффект пропадает вовсе.
		#define DDOF_FRINGE float(0.7) 		// коэффициент разброса пикселей при расчете хроматической аберрации. Чем больше поставить, тем больше эффект аберрации.
	#define DDOF_KERNEL float(1)			// коэффициент размытия. Влияет на интенсивность блюра	
	#define DDOF_THRESHOLD float(0.5) 		// порог яркости, ниже которого выборка пикселя будет отбрасываться. Более 1,1 не ставить. При увеличении, как правило, надо увеличивать DDOF_GAIN.
	#define DDOF_BIAS float(0.5) 			// коэффициент ослабления вклада от дальних выборок. При увеличении получается более резкий блюр.
	//far
	#define DDOF_FAR_PLANE float(100)		// дистанция от фокуса до границы сплошного блюра.
	#define DDOF_FAR_INTENSITY float(0.6)	// интенсивность эффекта. [0..1]
	//near
	#define DDOF_NEAR_PLANE float(20)		// дистанция, с которой блюр будет начинать уменьшаться. Больше поставите - дальше от актора будет распространяться ближний блюр
	#define DDOF_NEAR_MINDIST float (1.5)	// расстояние от камеры до плоскости начала заблюривания. Выставлять, чтобы не блюрилось оружие
	#define DDOF_NEAR_INTENSITY float(0.3)	// интенсивность эффекта. [0..1]
// zoom depth of field
	#define ZDOF_MINDIST float(250)	        // минимальная дистанция от центра экрана, в пикселях. Ближе к фокусу блюра нет
	#define ZDOF_MAXDIST float(500)			// максимальная дистанция от центра экрана, в пикселях. Дальше - равномерный блюр.
	#define ZDOF_OFFSET float(2)			// предел глубины пикселя, меньшие значения не учитываютя при расчете среднего фокуса.
// reload depth of field
	#define RDOF_DIST float (1)				// расстояние, на котором начинается блюр. Выставлять так, чтобы не блюрилось оружие (0,7-1,2)
	#define RDOF_SPEED float (5)			// скорость нарастания эффекта. Хорошие значения - 5-1
		
/////////////////////////////////////////////////////////////
// improved blur
	#define IMBLUR_START_DIST float(1.0)		// Начальная дистанция блюра
	#define IMBLUR_FINAL_DIST float(300)		// Конечная дистанция блюра
	#define IMBLUR_SAMPLES int(20)				// Количество семплов. Улучшает качество, ухудшает производительность
	#define IMBLUR_CLAMP float(0.01)			// Отсечка скорости поворота
	#define IMBLUR_SCALE_X float(-0.03)			// Граница скорости поворота по X
	#define IMBLUR_SCALE_Y float(0.03)			// Граница скорости поворота по Y
	#define IMBLUR_VEL_START float(0.001)		// Начальная скорость - включение эффекта
	#define IMBLUR_VEL_FIN float(0.02)			// Конечная скорость - выключение эффекта	
	
/////////////////////////////////////////////////////////////
// тепловизор
	#define IKV_DIST float(0.1)					// расстояние различимой геометрии
	#define IKV_HEATING_SPEED float(0.1)		// скорость нагрева/охлаждения оружия
	// пресеты
	#define IKV_PRESET_1_MIN float(0.274)		// пресеты подсвечиваемых материалов. Менять только в том случае, если знаете, что делаете.
	#define IKV_PRESET_1_MAX float(0.300)		// PRESET_1 - пресет для брони, костюмов, морд
	#define IKV_PRESET_2_MIN float(0.837)		// PRESET_2 - пресет для оружия
	#define IKV_PRESET_2_MAX float(0.862)		
	// цвета
	#define IKV_DEAD_COLOR float4(0.0,0.07,0.49,1.0)			// цветовая настройка тепловизора:
	#define IKV_MID3_COLOR float4(0.203,0.567,0.266,1.0)		// DEAD - цвет холодной геометрии
	#define IKV_MID2_COLOR float4(0.644,0.672,0.098,1.0)		// LIVE - цвет горячей геометрии
	#define IKV_MID1_COLOR float4(0.7,0.35,0.07,1.0)			// MID - промежуточные цвета
	#define IKV_LIVE_COLOR float4(0.7,0.16,0.08,1.0)	
	// помехи
	#define IKV_NOISE_INTENSITY float(1.0)		// интенсивность шума
	
/////////////////////////////////////////////////////////////
// Вода
	#define WATER_ENV_POWER float (0.7)				// интенсивность отражения скайкуба на воде
	#define PUDDLES_ENV_POWER float (0.3)			// интенсивность отражения скайкуба на лужах
	#define SW_USE_FOAM								// включить "пену" прибоя
	#define SW_FOAM_THICKNESS float (0.035)			// толщина "пены"
	#define SW_FOAM_INTENSITY float (3)				// интенсивность цвета "пены"
	#define SW_WATER_INTENSITY float (1.0)			// глубина цвета воды
	#define SW_REFL_INTENSITY float(2.0)			// интенсивность "честных" отражений
	#define SW_PUDDLES_REFL_INTENSITY float(3.0)	// интенсивность "честных" отражений в лужах
	#define MOON_ROAD_INTENSITY float(1.5)			// интенсивность "лунной дорожки"
	#define WATER_GLOSS float(0.5)					// интенсивность спекуляра на воде

/////////////////////////////////////////////////////////////
// Флары от ламп
	#define FL_POWER float(1.0)					// общая интенсивность фларов
	#define FL_GLOW_RADIUS float(0.2)			// радиус короны источника света
	#define FL_DIRT_INTENSITY float(1.0)		// интенсивность эффекта Lens Dirt
	
	#define MODEL_SELFLIGHT_POWER float(6.0)	// яркость свечения моделей

/////////////////////////////////////////////////////////////
// Эффекты, связанные с дождем

	#define RAIN_MAX_DIFFUSE_SCALING float(0.7)				// максимальный коэффициент затемнения геометрии
	#define RAIN_MAX_DIFFUSE_DETAILS_MULT float(0.9)		// коэффициент затемнения травы относительно террейна
	#define RAIN_MAX_SPECULAR_SCALING float (3.0)			// максимальный уровень повышения блика
	#define RAIN_MAX_REFLECTION_POWER float(0.1)			// максимальная сила отражений
	#define RAIN_WETTING_SPEED float(0.5)					// скорость намокания поверхностей
	#define RAIN_GAIN_REFLECTIONS float(3.0)				// усиление честных отражений на намокших поверхностях

	// test params, not used now
	#define RMAP_KERNEL float(1.0)
	#define RMAP_size float(2048)
	#define RMAP_MAX_ENV_MAP_COEFF float(0.1)

/////////////////////////////////////////////////////////////
// Волны по траве
//	#define USE_GRASS_WAVE							// включить "волны" от ветра по траве
	#define GRASS_WAVE_POWER float(2.0)				// "яркость", заметность волн
	#define GRASS_WAVE_FREQ float(0.7)				// частота появления волн

/////////////////////////////////////////////////////////////
// Освещение полупрозрачной геометрии
	#define TRANSLUCENT_GLOSS float(1.0)						// спекуляр
	#define TRANSLUCENT_TORCH_ATT_FACTOR float(0.001231148)		// calculated as 1/(torch_range*torch_range*0.95*0.95)
	#define TRANSLUCENT_TORCH_COLOR float4(0.6,0.64,0.60,0.8)	// color of torch light
	#define TRANSLUCENT_TORCH_POSITION float3(0.00,0.0,0)		// position of torch light in view space
	#define TRANSLUCENT_TORCH_ANGLE_COS float(0.8434)			// cosinus of a half of a torch cone angle

/////////////////////////////////////////////////////////////
// Интерактивные детекторы
							// ДЕТЕКТОР "ОТКЛИК"
	#define ID_DETECTOR_1_DETECT_RADIUS float(20.0)			// радиус детектирования артефактов. Менять одновременно вместе со значением в конфиге
	#define ID_DETECTOR_1_COLOR float4(1.0,1.0,1.0,1.0)		// цвет индикации
	#define ID_DETECTOR_1_POWER float(6.0)					// яркость индикации

							// ДЕТЕКТОР "МЕДВЕДЬ"
	#define ID_DETECTOR_2_DETECT_RADIUS float(20.0)			// радиус детектирования артефактов. Менять одновременно вместе со значением в конфиге
	#define ID_DETECTOR_2_COLOR float4(0.1,1.0,0.0,1.0)		// цвет индикации
	#define ID_DETECTOR_2_POWER float(5.0)					// яркость индикации
	#define ID_DETECTOR_2_CENTER float2(0.2559, 0.2305)		// текстурные координаты центра экрана детектора. Менять при смене текстуры
	#define ID_DETECTOR_2_SECTOR float(0.7)					// сектор индикации на экране. Чем больше значение, тем меньше сектор. Ровно 1.0 не ставить!

							// ДЕТЕКТОР "ВЕЛЕС"
	#define ID_DETECTOR_3_DETECT_RADIUS float(35.0)			// радиус детектирования артефактов. Менять одновременно вместе со значением в конфиге
	#define ID_DETECTOR_3_COLOR float4(0.1,1.0,0.0,1.0)		// цвет индикации
	#define ID_DETECTOR_3_POWER float(6.0)					// яркость индикации
	#define ID_DETECTOR_3_DOT_RADIUS float(0.01)			// радиус точки артефакта на экране
	#define ID_DETECTOR_3_SCREEN_CORNERS float4(0.4668, 0.8398, 0.1035, 0.2891)		// текстурные координаты углов экрана в формате (max u,max v,min u,min v). Менять при смене текстуры
	#define USE_ANOMALY_DETECTION							// включение режима индикации аномалий
	#define ID_DETECTOR_3_AN_COLOR float4(1.0,0.0,0.0,1.0)	// цвет индикации аномалий
	#define ID_DETECTOR_3_AN_DOT_RADIUS float(0.02)			// радиус точки аномалии на экране
	#define ID_DETECTOR_3_NUM_COLOR float4(1.0,0.0,0.0,1.0)	// цвет индикации цифр на экране

/////////////////////////////////////////////////////////////
// Разное
	#define DETAIL_TEXTURE_MULTIPLIER float(1.5)			// множитель яркости детальной текстуры
	
// Фильтр контраста
	#define CONTRAST_FILTER_COEF float(0.4)					// уровень контраста

// Цветокоррекция
	#define COLOR_GRADING_LUMINANCE float3(0.213, 0.715, 0.072)		// цвет для расчета относительной светимости. Лучше не менять.

/////////////////////////////////////////////////////////////
// FXAA
	#define FXAA_QUALITY__SUBPIX float(0.5)			// Choose the amount of sub-pixel aliasing removal.
													// This can effect sharpness.
													//   1.00 - upper limit (softer)
													//   0.75 - default amount of filtering
													//   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
													//   0.25 - almost off
													//   0.00 - completely off
	#define FXAA_QUALITY__EDGE_THRESHOLD float(0.063)    // The minimum amount of local contrast required to apply algorithm.
													//   0.333 - too little (faster)
													//   0.250 - low quality
													//   0.166 - default
													//   0.125 - high quality 
													//   0.063 - overkill (slower)
	#define FXAA_QUALITY__EDGE_THRESHOLD_MIN float(0.0312)    // Trims the algorithm from processing darks.
													//   0.0833 - upper limit (default, the start of visible unfiltered edges)
													//   0.0625 - high quality (faster)
													//   0.0312 - visible limit (slower)
													
// Screen space sunshafts
struct	v_ssss
{
	float4 P : POSITIONT;
	float2 tc0	: TEXCOORD0;
};

struct	v2p_ssss
{
	float2 tc0 : TEXCOORD0;
	float4 HPos : SV_Position;	// Clip-space position 	(for rasterization)
};
#endif