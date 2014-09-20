//=============================================================================
//  Filename:   UIColorAnimatorWrapper.h
//	Created by Roman E. Marchenko, vortex@gsc-game.kiev.ua
//	Copyright 2004. GSC Game World
//	---------------------------------------------------------------------------
//  Оберточка над LightAnimLibrary, в которой реализуется более удобная
//	работа с анимацией, и прикручен апдейт времени специфичный для UI: так
//	как UI апдейтится не на кажом кадре, то кормить функции интерполяции
//	Device.fTimeDelta нельзя - анимация тормознутая получается. Приходится 
//	постоянно считать время между апдейтами.
//=============================================================================
#pragma once

//////////////////////////////////////////////////////////////////////////

class CLAItem;

//////////////////////////////////////////////////////////////////////////

class CUIColorAnimatorWrapper
{
public:
	// Анимация сразу играется после создания объекта.
	// Чтобы начать ее заново используйте Reset()
	// Для проигрывания только 1 цикла вызовите Cyclic(false);

						CUIColorAnimatorWrapper	();
						~CUIColorAnimatorWrapper(){};
	// colorToModify - указатель на цвет который меняем
	// animationName - имя цветовой анимации
	explicit			CUIColorAnimatorWrapper	(const shared_str &animationName);
//						CUIColorAnimatorWrapper	(const shared_str &animationName, u32 *colorToModify);
						CUIColorAnimatorWrapper	(u32 *colorToModify);

	// Установить новую цветовую анимацию
	void				SetColorAnimation		(const shared_str &animationName);
	void				SetColorToModify		(u32 *colorToModify);
	// Функция которую необходимо обязательно поместить в Update диалога, для обновления анимации
	void				Update					();
	void				Cyclic					(bool cyclic)	{ isCyclic = cyclic; }
	void				Reset					();
	u32					GetColor				() const		{ return currColor; }
	int					LastFrame				() const		{ return currFrame; }
	int					TotalFrames				() const;
	bool				Done					() const		{ return isDone; }
	void				SetDone					(bool value)	{ isDone = value; }
	void				Reverese				(bool value);
	void				GoToEnd					();

private:
	// Собственно анимация
	CLAItem				*colorAnimation;
	// Предыдущее запоменное значение глобального времени игры
	float				prevGlobalTime;
	// Время прошедшее с начала анимации
	float				animationTime;
	// Циклическое бесконечное проигрывание
	bool				isCyclic;
	// Цвет который мы можем изменять переданный нам извне
	u32					*color;
	// Текущий цвет анимации
	u32					currColor;
	// Текущий кадр анимации
	int					currFrame;
	// Анимация закончилась?
	bool				isDone;
	// Анимация прокручивается в реверсивном порядке? Не применимо для бесконечной анимации
	bool				reverse;
	// Коеффициент для пересчета времени при реверснутом проигрывании
	float				kRev;

public:
	CLAItem				*GetAnimation			() const		{ return colorAnimation; } 
};
