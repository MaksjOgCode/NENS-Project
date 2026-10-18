import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved' / 'InspectionPromptFix'
out.mkdir(parents=True, exist_ok=True)
for path in [
    '/Game/NENS/UI/Interaction/WBP_InteractionPrompt',
    '/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase',
    '/Game/NENS/Environment/Interior/Blueprints/BP_Wardrobe',
]:
    bp = unreal.load_asset(path)
    assert bp, path
    (out / (path.rsplit('/', 1)[-1] + '.json')).write_text(
        unreal.InspectionEditorLibrary.dump_blueprint(bp), encoding='utf-8')
unreal.log('NENS_PROMPT_BUG_AUDIT_OK')
