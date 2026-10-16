"""Run once with UnrealEditor-Cmd -run=pythonscript -EnablePlugins=PythonScriptPlugin -script=<absolute path>."""
import unreal
from pathlib import Path
subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if (Path(unreal.Paths.project_content_dir()) / 'Maps/Startup.umap').exists():
    raise RuntimeError('Startup already exists; refusing to overwrite it')
subsystem.new_level('/Game/Maps/Startup')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100))
subsystem.save_current_level()
unreal.log('LUDUMDARE_MAP_CREATED')
