# ImGuiColorTextEdit
Syntax highlighting text editor for ImGui

![Screenshot](https://github.com/BalazsJako/ImGuiColorTextEdit/blob/master/ImGuiTextEdit.png "Screenshot")

Demo project: https://github.com/BalazsJako/ColorTextEditorDemo

This is my attempt to write a relatively simple widget which provides source code editing functionality with basic syntax highlighting.

While it relies on Omar Cornut's https://github.com/ocornut/imgui, it does not follow the "pure" one widget - one function approach. Since the editor has to maintain a relatively complex internal state, it did not seem to be practical to try and enforce fully immediate mode.

The code is work in progress, please report if you find any issues.

Main features are:
 - approximates typical code editor look and feel (essential mouse/keyboard commands work - I mean, the commands _I_ normally use :))
 - undo/redo support
 - extensible, multiple language syntax support
 - identifier declarations: a small piece of text associated with an identifier. The editor displays it in a tooltip when the mouse cursor is hovered over the identifier
 - error markers: the user can specify a list of error messages together the line of occurence, the editor will highligh the lines with red backround and display error message in a tooltip when the mouse cursor is hovered over the line
 - supports large files: there is no explicit limit set on file size or number of lines, performance is not affected when large files are loaded (except syntax coloring, see below)
 - color palette support: you can switch between different color palettes, or even define your own

Known issues:
 - syntax highligthing is based on std::regex, which is diasppointingly slow. Because of that, the highlighting process is amortized between multiple frames. Hand-written colorizers and/or a lexical scanner might help resolve this problem.
 - 8 bit character only, no Unicode or Utf support (yet)
 - no variable-width font support
 - there's no find/replace support

Don't forget to post your screenshots if you use this little piece of software in order to keep me motivated. :)
