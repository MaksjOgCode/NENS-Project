"""Run with the PythonScriptPlugin temporarily enabled on the command line."""
import unreal
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert level.load_level('/Game/Maps/Startup'), 'Cannot load Startup'
expected = {'r.ForwardShading': 1, 'r.AntiAliasingMethod': 5,
            'r.DefaultFeature.Bloom': 1, 'r.Nanite.ProjectEnabled': 0,
            'r.DynamicGlobalIlluminationMethod': 0, 'r.RayTracing': 0,
            'r.DefaultFeature.MotionBlur': 0, 'r.MotionBlurQuality': 0,
            'r.DepthOfFieldQuality': 0,
            'r.SupportExpFogMatchesVolumetricFog': 0}
for name, value in expected.items():
    actual = unreal.SystemLibrary.get_console_variable_int_value(name)
    assert actual == value, f'{name}: expected {value}, got {actual}'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for quality in range(5):
    unreal.SystemLibrary.execute_console_command(world, f'sg.PostProcessQuality {quality}')
    unreal.SystemLibrary.execute_console_command(world, f'sg.AntiAliasingQuality {quality}')
    assert unreal.SystemLibrary.get_console_variable_int_value('r.BloomQuality') == 3
    assert unreal.SystemLibrary.get_console_variable_int_value('r.SMAA.Quality') == (0 if quality == 0 else 1)
print('LUDUMDARE_VERIFIED: map, Forward, SMAA, Bloom, all five scalability tiers')
