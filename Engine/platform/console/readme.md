This directory implements the "console" port. It can be used to port AGS to a modern console. It doesn't do anything without your implementing some things independently privately. If someone wishes to implement targeting a homebrew platform using this framework, please consult with me to create an organizational pattern which won't clash with my work. I believe working solely out of a subdirectory of this directory may be a good idea.

Now I'm going to list what you do to make it work. At some point I will deliver an example really minimal and bad "console" port which actually just targets windows, so that you can use it as a template.

* Define CONSOLE_VERSION
* Implement something named like "AGSConsolePort", which is a AGSPlatformDriver, by studying the examples of other platform drivers
* Implement "mutex_console.h" and "thread_console.h" by studying examples, etc.
* ALLEGRO
** Create a configuration header file suitable for building allegro
** Privately patch allegro to include it in near the top in the platform list. (TODO: can we incorporate this into official AGS allegro fork?)
** Implement a certain set of allegro APIs (TBD). You will have to be creative here.

* Third Party Libraries:
** You must use AGS's alfont, and it's included freetype 2.1.3! Else some fonts in some people's games won't render correctly
** Incorproate libogg and libvorbis and libtheora somehow. 


OK -- I'm not through with this yet. I just needed to start it to collect my thoughts. I'll finish it later.
(remember to mention plugins, and possibly template-stub those too, or at least one of them)