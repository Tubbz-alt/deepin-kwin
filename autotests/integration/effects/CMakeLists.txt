if (XCB_ICCCM_FOUND)
    integrationTest(NAME testTranslucency SRCS translucency_test.cpp LIBS XCB::ICCCM)
    integrationTest(NAME testSlidingPopups SRCS slidingpopups_test.cpp LIBS XCB::ICCCM)
    integrationTest(NAME testShadeWobblyWindows SRCS wobbly_shade_test.cpp LIBS XCB::ICCCM)
endif()
integrationTest(NAME testFade SRCS fade_test.cpp)
integrationTest(WAYLAND_ONLY NAME testEffectWindowGeometry SRCS windowgeometry_test.cpp)
integrationTest(NAME testScriptedEffects SRCS scripted_effects_test.cpp)
integrationTest(WAYLAND_ONLY NAME testToplevelOpenCloseAnimation SRCS toplevel_open_close_animation_test.cpp)
integrationTest(WAYLAND_ONLY NAME testPopupOpenCloseAnimation SRCS popup_open_close_animation_test.cpp)
