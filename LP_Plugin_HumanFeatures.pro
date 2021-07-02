QT += gui widgets

TEMPLATE = lib
DEFINES += LP_PLUGIN_HUMANFEATURE_LIBRARY _USE_MATH_DEFINES ON_COMPILER_MSC OPENNURBS_IMPORTS NOMINMAX
DEFINES += EIGEN_HAS_CONSTEXPR EIGEN_MAX_CPP_VER=17

CONFIG += c++17

QMAKE_POST_LINK=$(MAKE) install

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    extern/eiquadprog-1.2.3/src/eiquadprog-fast.cpp \
    extern/eiquadprog-1.2.3/src/eiquadprog.cpp \
    lp_humanfeature.cpp \
    lp_pca_human.cpp

HEADERS += \
    LP_Plugin_HumanFeature_global.h \
    MeshPlaneIntersect.hpp \
    extern/eiquadprog.hpp \
    lp_humanfeature.h \
    lp_pca_human.h

# Default rules for deployment.
target.path = $$OUT_PWD/../App/plugins/$$TARGET

!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Model/release/ -lModel
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Model/debug/ -lModel
else:unix:!macx: LIBS += -L$$OUT_PWD/../Model/ -lModel

INCLUDEPATH += $$PWD/../Model
DEPENDPATH += $$PWD/../Model

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Functional/release/ -lFunctional
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Functional/debug/ -lFunctional
else:unix:!macx: LIBS += -L$$OUT_PWD/../Functional/ -lFunctional

INCLUDEPATH += $$PWD/../Functional
DEPENDPATH += $$PWD/../Functional

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCored
else:unix:!macx: LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore

INCLUDEPATH += $$PWD/../../OpenMesh/include
DEPENDPATH += $$PWD/../../OpenMesh/include

DISTFILES +=

INCLUDEPATH += $$PWD/extern/eigen-3.4-rc1/install/include/eigen3
DEPENDPATH += $$PWD/extern/eigen-3.4-rc1/install/include/eigen3

INCLUDEPATH += $$PWD/extern/eiquadprog-1.2.3/include
DEPENDPATH += $$PWD/extern/eiquadprog-1.2.3/include

INCLUDEPATH += $$PWD/extern/opennurbs
DEPENDPATH += $$PWD/extern/opennurbs

win32: LIBS += -L$$PWD/extern/opennurbs/install/lib -lopennurbs_public

win32: LIBS += -L$$PWD/extern/embree3/lib/ -lembree3
else:unix:!macx|win32: LIBS += -L$$PWD/extern/embree3/lib/ -lembree3

INCLUDEPATH += $$PWD/extern/embree3/include
DEPENDPATH += $$PWD/extern/embree3/include

unix: {
    SOURCES += \
        extern/opennurbs/opennurbs_3dm_attributes.cpp \
        extern/opennurbs/opennurbs_3dm_properties.cpp \
        extern/opennurbs/opennurbs_3dm_settings.cpp \
        extern/opennurbs/opennurbs_annotationbase.cpp \
        extern/opennurbs/opennurbs_apple_nsfont.cpp \
        extern/opennurbs/opennurbs_arc.cpp \
        extern/opennurbs/opennurbs_arccurve.cpp \
        extern/opennurbs/opennurbs_archive.cpp \
        extern/opennurbs/opennurbs_archive_manifest.cpp \
        extern/opennurbs/opennurbs_array.cpp \
        extern/opennurbs/opennurbs_base32.cpp \
        extern/opennurbs/opennurbs_base64.cpp \
        extern/opennurbs/opennurbs_beam.cpp \
        extern/opennurbs/opennurbs_bezier.cpp \
        extern/opennurbs/opennurbs_beziervolume.cpp \
        extern/opennurbs/opennurbs_bitmap.cpp \
        extern/opennurbs/opennurbs_bounding_box.cpp \
        extern/opennurbs/opennurbs_box.cpp \
        extern/opennurbs/opennurbs_brep.cpp \
        extern/opennurbs/opennurbs_brep_extrude.cpp \
        extern/opennurbs/opennurbs_brep_io.cpp \
        extern/opennurbs/opennurbs_brep_isvalid.cpp \
        extern/opennurbs/opennurbs_brep_region.cpp \
        extern/opennurbs/opennurbs_brep_tools.cpp \
        extern/opennurbs/opennurbs_brep_v2valid.cpp \
        extern/opennurbs/opennurbs_calculator.cpp \
        extern/opennurbs/opennurbs_circle.cpp \
        extern/opennurbs/opennurbs_color.cpp \
        extern/opennurbs/opennurbs_compress.cpp \
        extern/opennurbs/opennurbs_compstat.cpp \
        extern/opennurbs/opennurbs_cone.cpp \
        extern/opennurbs/opennurbs_convex_poly.cpp \
        extern/opennurbs/opennurbs_crc.cpp \
        extern/opennurbs/opennurbs_curve.cpp \
        extern/opennurbs/opennurbs_curveonsurface.cpp \
        extern/opennurbs/opennurbs_curveproxy.cpp \
        extern/opennurbs/opennurbs_cylinder.cpp \
        extern/opennurbs/opennurbs_date.cpp \
        extern/opennurbs/opennurbs_defines.cpp \
        extern/opennurbs/opennurbs_detail.cpp \
        extern/opennurbs/opennurbs_dimension.cpp \
        extern/opennurbs/opennurbs_dimensionformat.cpp \
        extern/opennurbs/opennurbs_dimensionstyle.cpp \
        extern/opennurbs/opennurbs_dll.cpp \
        extern/opennurbs/opennurbs_ellipse.cpp \
        extern/opennurbs/opennurbs_embedded_file.cpp \
        extern/opennurbs/opennurbs_error.cpp \
        extern/opennurbs/opennurbs_error_message.cpp \
        extern/opennurbs/opennurbs_evaluate_nurbs.cpp \
        extern/opennurbs/opennurbs_extensions.cpp \
        extern/opennurbs/opennurbs_file_utilities.cpp \
        extern/opennurbs/opennurbs_font.cpp \
        extern/opennurbs/opennurbs_freetype.cpp \
        extern/opennurbs/opennurbs_fsp.cpp \
        extern/opennurbs/opennurbs_function_list.cpp \
        extern/opennurbs/opennurbs_geometry.cpp \
        extern/opennurbs/opennurbs_glyph_outline.cpp \
        extern/opennurbs/opennurbs_group.cpp \
        extern/opennurbs/opennurbs_hash_table.cpp \
        extern/opennurbs/opennurbs_hatch.cpp \
        extern/opennurbs/opennurbs_instance.cpp \
        extern/opennurbs/opennurbs_internal_V2_annotation.cpp \
        extern/opennurbs/opennurbs_internal_V5_annotation.cpp \
        extern/opennurbs/opennurbs_internal_V5_dimstyle.cpp \
        extern/opennurbs/opennurbs_internal_Vx_annotation.cpp \
        extern/opennurbs/opennurbs_intersect.cpp \
        extern/opennurbs/opennurbs_ipoint.cpp \
        extern/opennurbs/opennurbs_knot.cpp \
        extern/opennurbs/opennurbs_layer.cpp \
        extern/opennurbs/opennurbs_leader.cpp \
        extern/opennurbs/opennurbs_light.cpp \
        extern/opennurbs/opennurbs_line.cpp \
        extern/opennurbs/opennurbs_linecurve.cpp \
        extern/opennurbs/opennurbs_linetype.cpp \
        extern/opennurbs/opennurbs_locale.cpp \
        extern/opennurbs/opennurbs_lock.cpp \
        extern/opennurbs/opennurbs_lookup.cpp \
        extern/opennurbs/opennurbs_material.cpp \
        extern/opennurbs/opennurbs_math.cpp \
        extern/opennurbs/opennurbs_matrix.cpp \
        extern/opennurbs/opennurbs_md5.cpp \
        extern/opennurbs/opennurbs_memory_util.cpp \
        extern/opennurbs/opennurbs_mesh.cpp \
        extern/opennurbs/opennurbs_mesh_ngon.cpp \
        extern/opennurbs/opennurbs_mesh_tools.cpp \
        extern/opennurbs/opennurbs_mesh_topology.cpp \
        extern/opennurbs/opennurbs_model_component.cpp \
        extern/opennurbs/opennurbs_model_geometry.cpp \
        extern/opennurbs/opennurbs_morph.cpp \
        extern/opennurbs/opennurbs_nurbscurve.cpp \
        extern/opennurbs/opennurbs_nurbssurface.cpp \
        extern/opennurbs/opennurbs_nurbsvolume.cpp \
        extern/opennurbs/opennurbs_object.cpp \
        extern/opennurbs/opennurbs_object_history.cpp \
        extern/opennurbs/opennurbs_objref.cpp \
        extern/opennurbs/opennurbs_offsetsurface.cpp \
        extern/opennurbs/opennurbs_optimize.cpp \
        extern/opennurbs/opennurbs_parse_angle.cpp \
        extern/opennurbs/opennurbs_parse_length.cpp \
        extern/opennurbs/opennurbs_parse_number.cpp \
        extern/opennurbs/opennurbs_parse_point.cpp \
        extern/opennurbs/opennurbs_parse_settings.cpp \
        extern/opennurbs/opennurbs_photogrammetry.cpp \
        extern/opennurbs/opennurbs_plane.cpp \
        extern/opennurbs/opennurbs_planesurface.cpp \
        extern/opennurbs/opennurbs_pluginlist.cpp \
        extern/opennurbs/opennurbs_point.cpp \
        extern/opennurbs/opennurbs_pointcloud.cpp \
        extern/opennurbs/opennurbs_pointgeometry.cpp \
        extern/opennurbs/opennurbs_pointgrid.cpp \
        extern/opennurbs/opennurbs_polycurve.cpp \
        extern/opennurbs/opennurbs_polyedgecurve.cpp \
        extern/opennurbs/opennurbs_polyline.cpp \
        extern/opennurbs/opennurbs_polylinecurve.cpp \
        extern/opennurbs/opennurbs_precompiledheader.cpp \
        extern/opennurbs/opennurbs_progress_reporter.cpp \
        extern/opennurbs/opennurbs_public_memory.cpp \
        extern/opennurbs/opennurbs_quaternion.cpp \
        extern/opennurbs/opennurbs_rand.cpp \
        extern/opennurbs/opennurbs_revsurface.cpp \
        extern/opennurbs/opennurbs_rtree.cpp \
        extern/opennurbs/opennurbs_sha1.cpp \
        extern/opennurbs/opennurbs_sleeplock.cpp \
        extern/opennurbs/opennurbs_sort.cpp \
        extern/opennurbs/opennurbs_sphere.cpp \
        extern/opennurbs/opennurbs_statics.cpp \
        extern/opennurbs/opennurbs_std_string_format.cpp \
        extern/opennurbs/opennurbs_std_string_utf.cpp \
        extern/opennurbs/opennurbs_string.cpp \
        extern/opennurbs/opennurbs_string_compare.cpp \
        extern/opennurbs/opennurbs_string_format.cpp \
        extern/opennurbs/opennurbs_string_scan.cpp \
        extern/opennurbs/opennurbs_string_values.cpp \
        extern/opennurbs/opennurbs_subd.cpp \
        extern/opennurbs/opennurbs_subd_archive.cpp \
        extern/opennurbs/opennurbs_subd_copy.cpp \
        extern/opennurbs/opennurbs_subd_data.cpp \
        extern/opennurbs/opennurbs_subd_eval.cpp \
        extern/opennurbs/opennurbs_subd_fragment.cpp \
        extern/opennurbs/opennurbs_subd_frommesh.cpp \
        extern/opennurbs/opennurbs_subd_heap.cpp \
        extern/opennurbs/opennurbs_subd_iter.cpp \
        extern/opennurbs/opennurbs_subd_limit.cpp \
        extern/opennurbs/opennurbs_subd_matrix.cpp \
        extern/opennurbs/opennurbs_subd_mesh.cpp \
        extern/opennurbs/opennurbs_subd_ref.cpp \
        extern/opennurbs/opennurbs_subd_ring.cpp \
        extern/opennurbs/opennurbs_subd_sector.cpp \
        extern/opennurbs/opennurbs_subd_texture.cpp \
        extern/opennurbs/opennurbs_sum.cpp \
        extern/opennurbs/opennurbs_sumsurface.cpp \
        extern/opennurbs/opennurbs_surface.cpp \
        extern/opennurbs/opennurbs_surfaceproxy.cpp \
        extern/opennurbs/opennurbs_symmetry.cpp \
        extern/opennurbs/opennurbs_terminator.cpp \
        extern/opennurbs/opennurbs_testclass.cpp \
        extern/opennurbs/opennurbs_text.cpp \
        extern/opennurbs/opennurbs_text_style.cpp \
        extern/opennurbs/opennurbs_textcontext.cpp \
        extern/opennurbs/opennurbs_textdraw.cpp \
        extern/opennurbs/opennurbs_textglyph.cpp \
        extern/opennurbs/opennurbs_textiterator.cpp \
        extern/opennurbs/opennurbs_textlog.cpp \
        extern/opennurbs/opennurbs_textobject.cpp \
        extern/opennurbs/opennurbs_textrun.cpp \
        extern/opennurbs/opennurbs_topology.cpp \
        extern/opennurbs/opennurbs_torus.cpp \
        extern/opennurbs/opennurbs_unicode.cpp \
        extern/opennurbs/opennurbs_unicode_cpsb.cpp \
        extern/opennurbs/opennurbs_units.cpp \
        extern/opennurbs/opennurbs_userdata.cpp \
        extern/opennurbs/opennurbs_userdata_obsolete.cpp \
        extern/opennurbs/opennurbs_uuid.cpp \
        extern/opennurbs/opennurbs_version.cpp \
        extern/opennurbs/opennurbs_version_number.cpp \
        extern/opennurbs/opennurbs_viewport.cpp \
        extern/opennurbs/opennurbs_win_dwrite.cpp \
        extern/opennurbs/opennurbs_workspace.cpp \
        extern/opennurbs/opennurbs_wstring.cpp \
        extern/opennurbs/opennurbs_xform.cpp \
        extern/opennurbs/opennurbs_zlib.cpp \
        extern/opennurbs/opennurbs_zlib_memory.cpp \
        extern/opennurbs/zlib/adler32.c \
        extern/opennurbs/zlib/compress.c \
        extern/opennurbs/zlib/crc32.c \
        extern/opennurbs/zlib/deflate.c \
        extern/opennurbs/zlib/infback.c \
        extern/opennurbs/zlib/inffast.c \
        extern/opennurbs/zlib/inflate.c \
        extern/opennurbs/zlib/inftrees.c \
        extern/opennurbs/zlib/trees.c \
        extern/opennurbs/zlib/uncompr.c \
        extern/opennurbs/zlib/zutil.c


    HEADERS += \
        extern/opennurbs/opennurbs.h \
        extern/opennurbs/opennurbs_3dm.h \
        extern/opennurbs/opennurbs_3dm_attributes.h \
        extern/opennurbs/opennurbs_3dm_properties.h \
        extern/opennurbs/opennurbs_3dm_settings.h \
        extern/opennurbs/opennurbs_annotationbase.h \
        extern/opennurbs/opennurbs_apple_nsfont.h \
        extern/opennurbs/opennurbs_arc.h \
        extern/opennurbs/opennurbs_arccurve.h \
        extern/opennurbs/opennurbs_archive.h \
        extern/opennurbs/opennurbs_array.h \
        extern/opennurbs/opennurbs_array_defs.h \
        extern/opennurbs/opennurbs_atomic_op.h \
        extern/opennurbs/opennurbs_base32.h \
        extern/opennurbs/opennurbs_base64.h \
        extern/opennurbs/opennurbs_beam.h \
        extern/opennurbs/opennurbs_bezier.h \
        extern/opennurbs/opennurbs_bitmap.h \
        extern/opennurbs/opennurbs_bounding_box.h \
        extern/opennurbs/opennurbs_box.h \
        extern/opennurbs/opennurbs_brep.h \
        extern/opennurbs/opennurbs_circle.h \
        extern/opennurbs/opennurbs_color.h \
        extern/opennurbs/opennurbs_compress.h \
        extern/opennurbs/opennurbs_compstat.h \
        extern/opennurbs/opennurbs_cone.h \
        extern/opennurbs/opennurbs_convex_poly.h \
        extern/opennurbs/opennurbs_cpp_base.h \
        extern/opennurbs/opennurbs_crc.h \
        extern/opennurbs/opennurbs_curve.h \
        extern/opennurbs/opennurbs_curveonsurface.h \
        extern/opennurbs/opennurbs_curveproxy.h \
        extern/opennurbs/opennurbs_cylinder.h \
        extern/opennurbs/opennurbs_date.h \
        extern/opennurbs/opennurbs_defines.h \
        extern/opennurbs/opennurbs_detail.h \
        extern/opennurbs/opennurbs_dimension.h \
        extern/opennurbs/opennurbs_dimensionformat.h \
        extern/opennurbs/opennurbs_dimensionstyle.h \
        extern/opennurbs/opennurbs_dll_resource.h \
        extern/opennurbs/opennurbs_ellipse.h \
        extern/opennurbs/opennurbs_error.h \
        extern/opennurbs/opennurbs_evaluate_nurbs.h \
        extern/opennurbs/opennurbs_extensions.h \
        extern/opennurbs/opennurbs_file_utilities.h \
        extern/opennurbs/opennurbs_font.h \
        extern/opennurbs/opennurbs_fpoint.h \
        extern/opennurbs/opennurbs_freetype.h \
        extern/opennurbs/opennurbs_freetype_include.h \
        extern/opennurbs/opennurbs_fsp.h \
        extern/opennurbs/opennurbs_fsp_defs.h \
        extern/opennurbs/opennurbs_function_list.h \
        extern/opennurbs/opennurbs_geometry.h \
        extern/opennurbs/opennurbs_group.h \
        extern/opennurbs/opennurbs_hash_table.h \
        extern/opennurbs/opennurbs_hatch.h \
        extern/opennurbs/opennurbs_hsort_template.h \
        extern/opennurbs/opennurbs_input_libsdir.h \
        extern/opennurbs/opennurbs_instance.h \
        extern/opennurbs/opennurbs_internal_V2_annotation.h \
        extern/opennurbs/opennurbs_internal_V5_annotation.h \
        extern/opennurbs/opennurbs_internal_V5_dimstyle.h \
        extern/opennurbs/opennurbs_internal_defines.h \
        extern/opennurbs/opennurbs_internal_glyph.h \
        extern/opennurbs/opennurbs_internal_unicode_cp.h \
        extern/opennurbs/opennurbs_intersect.h \
        extern/opennurbs/opennurbs_ipoint.h \
        extern/opennurbs/opennurbs_knot.h \
        extern/opennurbs/opennurbs_layer.h \
        extern/opennurbs/opennurbs_leader.h \
        extern/opennurbs/opennurbs_light.h \
        extern/opennurbs/opennurbs_line.h \
        extern/opennurbs/opennurbs_linecurve.h \
        extern/opennurbs/opennurbs_linestyle.h \
        extern/opennurbs/opennurbs_linetype.h \
        extern/opennurbs/opennurbs_locale.h \
        extern/opennurbs/opennurbs_lock.h \
        extern/opennurbs/opennurbs_lookup.h \
        extern/opennurbs/opennurbs_mapchan.h \
        extern/opennurbs/opennurbs_material.h \
        extern/opennurbs/opennurbs_math.h \
        extern/opennurbs/opennurbs_matrix.h \
        extern/opennurbs/opennurbs_md5.h \
        extern/opennurbs/opennurbs_memory.h \
        extern/opennurbs/opennurbs_mesh.h \
        extern/opennurbs/opennurbs_model_component.h \
        extern/opennurbs/opennurbs_model_geometry.h \
        extern/opennurbs/opennurbs_nurbscurve.h \
        extern/opennurbs/opennurbs_nurbssurface.h \
        extern/opennurbs/opennurbs_object.h \
        extern/opennurbs/opennurbs_object_history.h \
        extern/opennurbs/opennurbs_objref.h \
        extern/opennurbs/opennurbs_offsetsurface.h \
        extern/opennurbs/opennurbs_optimize.h \
        extern/opennurbs/opennurbs_parse.h \
        extern/opennurbs/opennurbs_photogrammetry.h \
        extern/opennurbs/opennurbs_plane.h \
        extern/opennurbs/opennurbs_planesurface.h \
        extern/opennurbs/opennurbs_pluginlist.h \
        extern/opennurbs/opennurbs_point.h \
        extern/opennurbs/opennurbs_pointcloud.h \
        extern/opennurbs/opennurbs_pointgeometry.h \
        extern/opennurbs/opennurbs_pointgrid.h \
        extern/opennurbs/opennurbs_polycurve.h \
        extern/opennurbs/opennurbs_polyedgecurve.h \
        extern/opennurbs/opennurbs_polyline.h \
        extern/opennurbs/opennurbs_polylinecurve.h \
        extern/opennurbs/opennurbs_private_wrap.h \
        extern/opennurbs/opennurbs_private_wrap_defs.h \
        extern/opennurbs/opennurbs_progress_reporter.h \
        extern/opennurbs/opennurbs_public.h \
        extern/opennurbs/opennurbs_public_examples.h \
        extern/opennurbs/opennurbs_public_version.h \
        extern/opennurbs/opennurbs_public_version.rc \
        extern/opennurbs/opennurbs_qsort_template.h \
        extern/opennurbs/opennurbs_quacksort_template.h \
        extern/opennurbs/opennurbs_quaternion.h \
        extern/opennurbs/opennurbs_rand.h \
        extern/opennurbs/opennurbs_rendering.h \
        extern/opennurbs/opennurbs_revsurface.h \
        extern/opennurbs/opennurbs_rtree.h \
        extern/opennurbs/opennurbs_sha1.h \
        extern/opennurbs/opennurbs_sleeplock.h \
        extern/opennurbs/opennurbs_sphere.h \
        extern/opennurbs/opennurbs_std_string.h \
        extern/opennurbs/opennurbs_string.h \
        extern/opennurbs/opennurbs_string_value.h \
        extern/opennurbs/opennurbs_subd.h \
        extern/opennurbs/opennurbs_subd_data.h \
        extern/opennurbs/opennurbs_sumsurface.h \
        extern/opennurbs/opennurbs_surface.h \
        extern/opennurbs/opennurbs_surfaceproxy.h \
        extern/opennurbs/opennurbs_symmetry.h \
        extern/opennurbs/opennurbs_system.h \
        extern/opennurbs/opennurbs_system_compiler.h \
        extern/opennurbs/opennurbs_system_runtime.h \
        extern/opennurbs/opennurbs_terminator.h \
        extern/opennurbs/opennurbs_testclass.h \
        extern/opennurbs/opennurbs_text.h \
        extern/opennurbs/opennurbs_text_style.h \
        extern/opennurbs/opennurbs_textcontext.h \
        extern/opennurbs/opennurbs_textdraw.h \
        extern/opennurbs/opennurbs_textglyph.h \
        extern/opennurbs/opennurbs_textiterator.h \
        extern/opennurbs/opennurbs_textlog.h \
        extern/opennurbs/opennurbs_textobject.h \
        extern/opennurbs/opennurbs_textrun.h \
        extern/opennurbs/opennurbs_texture.h \
        extern/opennurbs/opennurbs_texture_mapping.h \
        extern/opennurbs/opennurbs_topology.h \
        extern/opennurbs/opennurbs_torus.h \
        extern/opennurbs/opennurbs_unicode.h \
        extern/opennurbs/opennurbs_userdata.h \
        extern/opennurbs/opennurbs_uuid.h \
        extern/opennurbs/opennurbs_version.h \
        extern/opennurbs/opennurbs_version_number.h \
        extern/opennurbs/opennurbs_viewport.h \
        extern/opennurbs/opennurbs_win_dwrite.h \
        extern/opennurbs/opennurbs_windows_targetver.h \
        extern/opennurbs/opennurbs_wip.h \
        extern/opennurbs/opennurbs_workspace.h \
        extern/opennurbs/opennurbs_xform.h \
        extern/opennurbs/opennurbs_zlib.h \
        extern/opennurbs/zlib/crc32.h \
        extern/opennurbs/zlib/deflate.h \
        extern/opennurbs/zlib/inffast.h \
        extern/opennurbs/zlib/inffixed.h \
        extern/opennurbs/zlib/inflate.h \
        extern/opennurbs/zlib/inftrees.h \
        extern/opennurbs/zlib/trees.h \
        extern/opennurbs/zlib/zconf.h \
        extern/opennurbs/zlib/zlib.h \
        extern/opennurbs/zlib/zutil.h
}
