# Productivity-Watch
Productivity Watch is an open source software designed to monitor and record the duration of time users spend on various applications while running on a Windows system. This tool captures application usage data discreetly and compiles it into insightful reports for users. 
Productivity Watch is developed by me only and I don't consider myself an expert at designing software, so if you want to contribute to this project feel free to do so. I am going to improve this software and introduce new features. 
This is a standalone software, it does not need any web browser or internet connection to function and uses the Win32 API and other open source libraries. **It does not share the data it records locally.**

$${\color{red} Right \space now \space you \space need \space to \space compile \space two \space things \space separately \space, \space the \space pw-server.cpp \space using \space the \space g++ \space command }$$

$${\color{red} and \space the \space GUI \space frontend \space by \space running \space make. \newline I \space will \space be \space updating \space the \space project \space structure \space soon }$$

$${\color{red} and \space this \space won't \space be \space an \space issue.}$$

## Libraries-used
- [ImGui](https://github.com/ocornut/imgui)
- glfw
- [Implot](https://github.com/epezent/implot)
- [nlohmann json](https://github.com/nlohmann/json)

## Screenshots
Do note that this software is still WIP
<img alt="GUI" src="https://i.imgur.com/OKQHPQ6.png">
<img alt="GUI" src="https://i.imgur.com/ir4NhbD.png">

## Features to be added
- Let the user set time limits to certain apps and productivity watch will automatically close the apps after it sends a notification to the user.
- Display the usage of a set of apps over a date range.
- Detect whether the user is idle (away from mouse or keyboard).
  
# [About Me](https://github.com/Abhirup27/Abhirup27/blob/main/README.md)
