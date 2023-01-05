#pragma once

void ShowEditor();
bool IsEditor();
bool Editor_KeyPress(int key);
bool Editor_KeyRelease(int key);
bool Editor_KeyHold(int key);
bool Editor_MouseMove(int dx, int dy);
bool Editor_MouseWheel(int direction);
