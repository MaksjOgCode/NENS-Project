"""Run in UE's Python commandlet; reads assets without saving them."""
import json
from pathlib import Path
import unreal

out = Path(unreal.Paths.project_dir()) / 'Saved' / 'InspectionImplementation'
out.mkdir(parents=True, exist_ok=True)
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
paths = [
    '/Game/NENS/Gameplay/Interaction/Blueprints/BP_InteractableBase',
    '/Game/NENS/Gameplay/Interaction/Components/BPC_Interaction',
    '/Game/NENS/Environment/Interior/Blueprints/BP_Wardrobe',
    '/Game/NENS/Characters/Player/Blueprints/BP_PlayerCharacter',
    '/Game/NENS/UI/Inspection/WBP_InspectionScene',
]
for path in paths:
    bp = unreal.load_asset(path)
    assert bp, path
    dump = unreal.InspectionEditorLibrary.dump_blueprint(bp)
    (out / (path.rsplit('/', 1)[-1] + '.json')).write_text(dump, encoding='utf-8')
assets = [str(a.package_name) for a in registry.get_assets_by_path('/Game/NENS/UI/Inspection', recursive=True)]
(out / 'inspection-assets.json').write_text(json.dumps(assets, indent=2), encoding='utf-8')
print('NENS_INSPECTION_AUDIT_OK')
