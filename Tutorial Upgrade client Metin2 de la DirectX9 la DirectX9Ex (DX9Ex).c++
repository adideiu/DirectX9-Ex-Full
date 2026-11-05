Mulțumim.

După ce puneți DX9, puteți face ușor upgrade la DX9EX.

Hide Spoiler
Tutorial: Upgrade client Metin2 de la DirectX9 la DirectX9Ex (DX9Ex)

Pentru cine: ai deja un client pe DX9 (dacă ești pe DX8, fă întâi saltul DX8→DX9). Testat pe client VS2022 (toolset v143) / C++17.



De ce DX9Ex?
Nu mai există „device lost". Dispar ecranul negru / crash-urile la Alt-Tab / minimizare în fullscreen.
Performanță mai bună în fereastră (model flip / DWM) și latență mai mică.
Management de resurse mai stabil.
Cerințe
Windows Vista sau mai nou (WDDM). DX9Ex NU există pe XP. Dacă mai ai jucători pe XP, păstrează un build DX9 ca rezervă.
d3d9.h din orice Windows SDK modern (VS2019/2022) are deja interfețele Ex (IDirect3D9Ex, IDirect3DDevice9Ex, Direct3DCreate9Ex). Nu-ți trebuie SDK extra.
⚠ REGULA DE AUR
D3DPOOL_MANAGED NU există în DX9Ex. Fiecare resursă trebuie creată în D3DPOOL_DEFAULT. Un singur MANAGED uitat = create eșuat la runtime = geometrie/textură invizibilă sau crash. Trebuie convertite TOATE, nu doar cele evidente.
De ce e safe: un device 9Ex nu pierde niciodată suprafața și păstrează resursele DEFAULT peste ResetEx, deci NU mai trebuie să le recreezi la reset ca la DX9 simplu.



PASUL 1 — Tipurile de interfață (LPDIRECT3D9 → LPDIRECT3D9EX)

Fișier: EterLib/GrpBase.h și EterLib/GrpBase.cpp
Caută declarațiile membrilor și schimbă DOAR tipul (nu atinge GetDevice()):
Code

static LPDIRECT3D9              ms_lpd3d;
static LPDIRECT3DDEVICE9        ms_lpd3dDevice;
Înlocuiește cu:
Code

static LPDIRECT3D9EX            ms_lpd3d;
static LPDIRECT3DDEVICE9EX      ms_lpd3dDevice;
(la fel, în .cpp, la definiția statică: LPDIRECT3D9 → LPDIRECT3D9EX și LPDIRECT3DDEVICE9 → LPDIRECT3DDEVICE9EX)

Fișier: EterPythonLib/PythonGraphic.h și .cpp
Caută:
Code

LPDIRECT3D9 GetD3D()
Înlocuiește cu:
Code

LPDIRECT3D9EX GetD3D()
(și în .h, și în .cpp la definiția funcției)



PASUL 2 — Crearea device-ului (EterLib/GrpDevice.cpp)

2.1 Direct3DCreate9 → Direct3DCreate9Ex
Caută:
Code

ms_lpd3d = Direct3DCreate9(D3D_SDK_VERSION);
Înlocuiește cu:
Code

ms_lpd3d = NULL;
HRESULT hrCreateEx = Direct3DCreate9Ex(D3D_SDK_VERSION, &ms_lpd3d);
(verificarea existentă „if (!ms_lpd3d) return CREATE_NO_DIRECTX;" rămâne și prinde eșecul.)

2.2 CreateDevice → CreateDeviceEx
Înainte de apelul CreateDevice, ADAUGĂ construirea unui D3DDISPLAYMODEEX pentru fullscreen.
Caută:
Code

if (FAILED(ms_hLastResult = ms_lpd3d->CreateDevice(
Înlocuiește cu:
Code


D3DDISPLAYMODEEX fmEx; ZeroMemory(&fmEx, sizeof(fmEx));
if (!Windowed) {
    fmEx.Size = sizeof(D3DDISPLAYMODEEX);
    fmEx.Width = iHres; fmEx.Height = iVres;
    fmEx.RefreshRate = iReflashRate;
    fmEx.Format = pkD3DModeInfo->m_eD3DFmtPixel;
    fmEx.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
}
if (FAILED(ms_hLastResult = ms_lpd3d->CreateDeviceEx(
Apoi, la lista de argumente, ADAUGĂ parametrul de displaymode (NULL în fereastră, &fmEx în fullscreen).
Caută:
Code

&ms_d3dPresentParameter,
                &ms_lpd3dDevice)))
Înlocuiește cu:
Code

&ms_d3dPresentParameter,
                Windowed ? NULL : &fmEx,
                &ms_lpd3dDevice)))
2.3 Reset → ResetEx (funcția CGraphicDevice::Reset)
Caută:
Code

if (FAILED(hr = ms_lpd3dDevice->Reset(&ms_d3dPresentParameter)))
Înlocuiește cu:
Code


D3DDISPLAYMODEEX dmEx = {};
dmEx.Size = sizeof(D3DDISPLAYMODEEX);
dmEx.Width = ms_d3dPresentParameter.BackBufferWidth;
dmEx.Height = ms_d3dPresentParameter.BackBufferHeight;
dmEx.Format = ms_d3dPresentParameter.BackBufferFormat;
dmEx.RefreshRate = ms_d3dPresentParameter.FullScreen_RefreshRateInHz;
dmEx.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
if (FAILED(hr = ms_lpd3dDevice->ResetEx(&ms_d3dPresentParameter, ms_d3dPresentParameter.Windowed ? NULL : &dmEx)))


PASUL 3 — Enumerarea modurilor de afișare
Fișiere: EterLib/GrpDetector.cpp + .h, EterLib/GrpScreen.cpp, UserInterface/PythonSystem.cpp

Regula generală:
Tipul D3DDISPLAYMODE → D3DDISPLAYMODEEX (și mereu setează .Size = sizeof(D3DDISPLAYMODEEX) înainte de folosire)
Referințele IDirect3D9& / IDirect3DDevice9& → IDirect3D9Ex& / IDirect3DDevice9Ex&
Apelurile devin „Ex": GetAdapterDisplayModeEx, GetAdapterModeCountEx, EnumAdapterModesEx (ultimele două au nevoie de un D3DDISPLAYMODEFILTER)
Exemplu — caută:
Code

UINT n = rkD3D.GetAdapterModeCount(adapter, format);
Înlocuiește cu:
Code

D3DDISPLAYMODEFILTER filter = {};
filter.Size = sizeof(D3DDISPLAYMODEFILTER);
filter.Format = format;
UINT n = rkD3D.GetAdapterModeCountEx(adapter, &filter);
Exemplu — caută:
Code

rkD3D.EnumAdapterModes(adapter, format, i, &mod);
Înlocuiește cu (mod fiind acum D3DDISPLAYMODEEX):
Code

mod.Size = sizeof(D3DDISPLAYMODEEX);
rkD3D.EnumAdapterModesEx(adapter, &filter, i, &mod);
Exemplu — caută:
Code

GetAdapterDisplayMode(adapter, &mod)
Înlocuiește cu:
Code

GetAdapterDisplayModeEx(adapter, &mod, NULL)
(cu mod.Size setat înainte)

⚠ CAPCANĂ (m-a costat o eroare de compilare)
Dacă faci înlocuirea D3DDISPLAYMODE → D3DDISPLAYMODEEX cu un tool de „replace all", FĂ-O CASE-SENSITIVE. Tipul scris cu majuscule D3DDISPLAYMODE e subșir în nume de metode camelCase precum GetDesktopD3DDisplayModer, FIsEqualD3DDisplayMode, CompareD3DDisplayModeOrder. Un replace case-insensitive le strică numele → error C2039: '...' is not a member of 'D3D_CAdapterInfo'.
De asemenea, NU atinge constanta D3DDISPLAYMODE_MAX (n-o transforma în D3DDISPLAYMODEEX_MAX).



PASUL 4 — PARTEA IMPORTANTĂ: pool-ul D3DPOOL_MANAGED → D3DPOOL_DEFAULT

Caută în TOT proiectul D3DPOOL_MANAGED și convertește fiecare apariție. Două tipare:

A) Geometrie statică (umplută o singură dată) — schimbi doar pool-ul:
Caută:
Code

D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED
Înlocuiește cu:
Code

D3DUSAGE_WRITEONLY, D3DPOOL_DEFAULT
Fișiere tipice: GrpVertexBufferStatic.cpp, GrpIndexBuffer.cpp, GrpDevice.cpp (index buffer-ul default), GameLib/DungeonBlock.cpp, GameLib/SnowEnvironment.cpp, GameLib/TerrainPatch.cpp, UserInterface/PythonMiniMap.cpp.

B) Resurse dinamice / care se actualizează / se fac Lock — adaugi D3DUSAGE_DYNAMIC:
Caută:
Code

D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED
Înlocuiește cu:
Code

D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT
Fișiere tipice: EterGrnLib/LODController.cpp, EterGrnLib/ModelInstanceModel.cpp, EterGrnLib/Model.cpp.
Texturile care se fac Lock/Update (A8R8G8B8 / A4R4G4B4, splat de teren) → pun D3DUSAGE_DYNAMIC + D3DPOOL_DEFAULT: EterLib/BlockTexture.cpp, EterLib/GrpImageTexture.cpp, GameLib/AreaTerrain.cpp.

⚠ NU te baza orbește pe niciun diff/patch!
Majoritatea patch-urilor publicate sunt făcute pe o altă sursă și ratează lucruri pe care tu le ai. Caută în PROPRIA sursă.
SpeedTree (SpeedTreeLib/SpeedTreeWrapper.cpp): vertex/index buffer-ele de branch, frond și leaf sunt de obicei create cu D3DPOOL_MANAGED și lipsesc din diff-uri. Dacă le uiți → copacii dispar sau clientul crapă. Sunt buffere statice → tiparul A.
GrpImageTexture.cpp dacă are logică condiționată de pool (ex: m_bDynamic ? DEFAULT : MANAGED sau ms_bSupportDXT ? MANAGED : SCRATCH): rescrie ramura MANAGED să folosească DEFAULT.
Caz special: texturi DDS (foarte des ratat)
Dacă codul tău face LockRect pe textura DDS ca s-o umple, o textură DEFAULT simplă nu se poate face Lock. Soluția corectă în 9Ex: creează o textură intermediară în D3DPOOL_SYSTEMMEM (e lockabilă, inclusiv pentru formate comprimate DXT), umple-o, apoi creează textura finală în D3DPOOL_DEFAULT și copiaz-o cu UpdateTexture:
Code


// textura intermediara (lockabila) ramane SYSTEMMEM:
D3DPOOL pool = ms_bSupportDXT ? D3DPOOL_SYSTEMMEM : D3DPOOL_SCRATCH;
// ... LockRect + Copy in lpd3dTexture (staging) ...
// apoi urci pe GPU intr-o textura DEFAULT:
LPDIRECT3DTEXTURE9 lpDefaultTex = NULL;
D3DXCreateTexture(ms_lpd3dDevice, w, h, mips, 0, format, D3DPOOL_DEFAULT, &lpDefaultTex);
ms_lpd3dDevice->UpdateTexture(lpd3dTexture, lpDefaultTex);
lpd3dTexture->Release();
m_lpd3dTexture = lpDefaultTex;


PASUL 5 — Compilare & testare
După conversie, clientul trebuie să compileze curat (singura eroare probabilă e cea cu replace-ul case-insensitive de la Pasul 3 — rezolv-o și merge).

Testează exact zonele atinse:
Pornește jocul + meniu + login se randează
Copacii (SpeedTree) pe o hartă cu pădure, în mișcare + tranziții LOD ← #1 lucru de verificat
Teren + texturi teren, apă, dungeon, hartă cu zăpadă
Minimap + tot UI-ul
Schimbare de rezoluție (testează ResetEx) + toggle fereastră/fullscreen
Alt-Tab în fullscreen de mai multe ori — NU trebuie ecran negru/crash (ăsta-i tot scopul)
Pe GPU slab: atenție la D3DERR_OUTOFVIDEOMEMORY (DEFAULT trăiește în VRAM)
Verificare „chiar rulez DX9Ex?"
d3d9.dll e aceeași pentru DX9 și 9Ex, deci nu te uiți la DLL-uri. După crearea device-ului, pune o verificare:
Code

IDirect3DDevice9Ex* pEx = NULL;
bool bIsEx = SUCCEEDED(ms_lpd3dDevice->QueryInterface(__uuidof(IDirect3DDevice9Ex), (void**)&pEx)) && pEx;
if (pEx) pEx->Release();
// scrie bIsEx intr-un log/fisier sau Tracen
Sau extern: API Monitor (rohitab) → vezi dacă se apelează Direct3DCreate9Ex (DX9Ex) vs Direct3DCreate9 (DX9 simplu).



Troubleshooting
Modele/copaci invizibili → ai uitat un D3DPOOL_MANAGED undeva; caută din nou (mai ales SpeedTree).
CreateDeviceEx → D3DERR_INVALIDCALL → în fullscreen trebuie un D3DDISPLAYMODEEX* valid (Width/Height/Format/RefreshRate dintr-un mod real al adaptorului).
Texturi negre/albe/stricate → calea DDS; folosește staging SYSTEMMEM → UpdateTexture → DEFAULT.
Crash la schimbarea rezoluției → vreo resursă DEFAULT tratată greșit la reset; cu 9Ex în general NU recreezi nimic — scoate vechiul „dans" de device-lost.
Nu pornește pe o mașină → aia e XP / fără WDDM. DX9Ex cere Vista+.

Tutorial bazat pe o implementare reală, validată: clientul compilează curat (Release/Win32, VS2022 v143) și rulează confirmat pe Direct3D9Ex.

diff original pe care e bazat tutorialul dacă nu vă descurcați: https://1024terabox.com/s/1XR9ALuF0D88EhwynIumW4A

src-binary-dx8-dx9-dx9ex-pentru-martysama-t5886 Aici aveti si Marty cu dx9ex implementat pt model