#
# Aspia Project
# Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

include(translations)

list(APPEND SOURCE_ROUTER_KEYGEN
    main.cc
    main_window.cc
    main_window.h
    main_window.ui
    keygen.rc)

list(APPEND SOURCE_ROUTER_KEYGEN_RESOURCES resources/keygen.qrc)

source_group("" FILES ${SOURCE_ROUTER_KEYGEN})
source_group(moc FILES ${SOURCE_ROUTER_KEYGEN_MOC})
source_group(resources FILES ${SOURCE_ROUTER_KEYGEN_RESOURCES})

if (WIN32)
    set(ROUTER_KEYGEN_PLATFORM_LIBS
        crypt32
        dwmapi
        imm32
        netapi32
        userenv
        uxtheme
        version
        winmm
        wtsapi32)
endif()

if (LINUX)
    set(ROUTER_KEYGEN_PLATFORM_LIBS
        stdc++fs
        optimized ${ICUUC_RELEASE}
        optimized ${ICUDATA_RELEASE}
        debug ${ICUUC_DEBUG}
        debug ${ICUDATA_DEBUG})
endif()

if (APPLE)
    set(ROUTER_KEYGEN_PLATFORM_LIBS
        optimized ${ICUUC_RELEASE}
        optimized ${ICUDATA_RELEASE}
        debug ${ICUUC_DEBUG}
        debug ${ICUDATA_DEBUG})

    set(aspia_router_keygen_ICON ${PROJECT_SOURCE_DIR}/router/keygen/resources/main.icns)
    set(aspia_router_keygen_PLIST ${PROJECT_SOURCE_DIR}/router/keygen/Info.plist)

    set_source_files_properties(${aspia_router_keygen_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

add_executable(aspia_router_keygen MACOSX_BUNDLE
    ${aspia_router_keygen_ICON}
    ${SOURCE_ROUTER_KEYGEN}
    ${SOURCE_ROUTER_KEYGEN_RESOURCES})

if (WIN32)
    set_target_properties(aspia_router_keygen PROPERTIES WIN32_EXECUTABLE TRUE)
    set_target_properties(aspia_router_keygen PROPERTIES LINK_FLAGS "/MANIFEST:NO")
endif()

if (APPLE)
    set_target_properties(aspia_router_keygen PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${aspia_router_keygen_PLIST})
    set_target_properties(aspia_router_keygen PROPERTIES MACOSX_BUNDLE_ICONFILE ${aspia_router_keygen_ICON})
endif()

target_link_libraries(aspia_router_keygen
    aspia_base
    aspia_qt_base
    ${ROUTER_KEYGEN_PLATFORM_LIBS}
    ${QT_COMMON_LIBS}
    ${QT_PLATFORM_LIBS}
    ${THIRD_PARTY_LIBS})
set_property(TARGET aspia_router_keygen PROPERTY AUTOMOC ON)
set_property(TARGET aspia_router_keygen PROPERTY AUTOUIC ON)
set_property(TARGET aspia_router_keygen PROPERTY AUTORCC ON)
qt5_import_plugins(aspia_router_keygen
    INCLUDE ""
    EXCLUDE ""
    INCLUDE_BY_TYPE ""
    EXCLUDE_BY_TYPE imageformats)
