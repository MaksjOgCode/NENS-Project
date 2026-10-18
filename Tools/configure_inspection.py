"""One-time/idempotent UE editor setup. Back up assets before first use."""
import json
from pathlib import Path
import unreal

result = json.loads(unreal.InspectionEditorLibrary.configure_inspection())
out = Path(unreal.Paths.project_dir()) / 'Saved' / 'InspectionImplementation'
out.mkdir(parents=True, exist_ok=True)
(out / 'setup-result.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
assert result.get('success'), result
unreal.log('NENS_INSPECTION_SETUP_OK ' + json.dumps(result))
