#include "Module.h"

extern ke::Vector<CWeapon> Weapons;
extern ke::Vector<CAmmo> Ammos;
extern ke::Vector<CProjectile> Projectiles;
extern ke::Vector<ItemInfo> ItemInfoes;

extern BOOL CanPrecache;
extern BOOL SV_Cheats;
extern BOOL MP_FriendlyFire;
extern cvar_t *CVar_LogPointer;

char PathAddOn[32] = "cswm";

cell WeaponCount = NULL;
cell AmmoCount = AMMO_MAX_TYPES;
cell ProjectileCount = NULL;

const cell WEAPON_TYPE_ID[] = { CSW_P228, CSW_XM1014, CSW_AK47, CSW_AWP };
const char WEAPON_TYPE_NAME[][8] = { "Pistol", "Shotgun", "Rifle", "Sniper" };
const float WEAPON_DEFAULT_DELAY[] = { 0.f, 0.2f, 0.f, 1.3f, 0.f, 0.3f, 0.f, 0.1f, 0.1f, 0.f, 0.1f, 0.2f, 0.1f, 0.3f, 0.1f, 0.1f, 0.1f, 0.2f, 1.5f, 0.1f, 0.1f, 0.9f, 0.1f, 0.1f, 0.3f, 0.0f, 0.2f, 0.1f, 0.1f, 0.f, 0.1f };
int MI_Trail, MI_Explosion, MI_Smoke;

void PrecacheModule(void)
{
	MI_Trail = PRECACHE_MODEL("sprites/laserbeam.spr");
	MI_Explosion = PRECACHE_MODEL("sprites/fexplo.spr");
	MI_Smoke = PRECACHE_MODEL("sprites/steam1.spr");
}

void StringToLower(char *String)
{
	for (unsigned int Index = NULL; Index < strlen(String); Index++)
		String[Index] = tolower(String[Index]);
}

cell AMX_NATIVE_CALL CreateWeapon(AMX *amx, cell *params)
{
	if (!CanPrecache)
	{
		LOG_CONSOLE(PLID, "[CSWM] Can Not Register Weapon At This Time.");
		return NULL;
	}

	CWeapon Weapon;
	memset(&Weapon, NULL, sizeof(CWeapon));

	char Model[MAX_MODEL_NAME], Path[MAX_MODEL_PATH_NAME];

	cell Length;
	strcpy(Model, MF_GetAmxString(amx, params[1], NULL, &Length));
	Weapon.Model = STRING(ALLOC_STRING(Model));
	Length = sprintf(Path, "models/%s/%s/V.mdl", PathAddOn, Model);
	PRECACHE_MODEL(STRING(Weapon.VModel = ALLOC_STRING(Path)));
	Path[Length - 5] = 'P';
	PRECACHE_MODEL(STRING(Weapon.PModel = ALLOC_STRING(Path)));
	Path[Length - 5] = 'W';
	PRECACHE_MODEL(Weapon.WModel = STRING(ALLOC_STRING(Path)));
	StringToLower(Model);
	sprintf(Path, "sprites/weapon_%s.txt", Model);
	PRECACHE_GENERIC(STRING(ALLOC_STRING(Path)));
	Weapon.Name = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[3], NULL, NULL)));
	WType Type = (WType)params[2];
	Weapon.Type = Type;
	Weapon.ID = WEAPON_TYPE_ID[Type];
	Weapon.Speed = 250.f;
	Weapons.append(Weapon);

	if (CVar_LogPointer->value)
		LOG_CONSOLE(PLID, "[CSWM] Creating Weapon: %s (Type=%s)", Weapon.Name, WEAPON_TYPE_NAME[Weapon.Type]);

	WeaponCount++;
	return WeaponCount - 1;
}

void UpdateAmmoList();
cell AMX_NATIVE_CALL CreateAmmo(AMX *amx, cell *params)
{
	if (!Ammos.length())
		UpdateAmmoList();

	CAmmo Ammo;
	Ammo.Cost = params[1];
	Ammo.Amount = params[2];
	Ammo.Max = params[3];
	Ammos.append(Ammo);

	AmmoCount++;
	return AmmoCount - 1;
}

cell AMX_NATIVE_CALL BuildWeaponFireSound(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	CWeapon *Weapon = &Weapons[Index];
	cell Length;
	char *String = MF_GetAmxString(amx, params[2], NULL, &Length);
	Weapon->FireSound = STRING(ALLOC_STRING(String));
	PRECACHE_SOUND(Weapon->FireSound);
	return NULL;
}

enum WAmmoType
{
	_None,
	_338Magnum,
	_762Nato,
	_556NatoBox,
	_556Nato,
	_BuckShot,
	_45ACP,
	_57MM,
	_50AE,
	_357SIG,
	_9MM,
	_FLASHBANG,
	_HEGRENADE,
	_SMOKEGRENADE,
	_C4,
};

cell AMX_NATIVE_CALL BuildWeaponAmmunition(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	CWeapon *Weapon = &Weapons[Index];
	Weapon->Clip = params[2];
	Weapon->AmmoID = params[3];
	return NULL;
}

cell AMX_NATIVE_CALL BuildWeaponDeploy(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	CWeapon *Weapon = &Weapons[Index];
	Weapon->AnimD = params[2];
	Weapon->AnimD_Duration = amx_ctof(params[3]);
	return NULL;
}

cell AMX_NATIVE_CALL BuildWeaponShoot(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	cell *AnimS = (cell *)MF_GetAmxAddr(amx, params[2]);

	if (!AnimS || !(AnimS + 1) || !(AnimS + 2))
		return NULL;

	CWeapon *Weapon = &Weapons[Index];
	Weapon->AnimS[0] = AnimS[0];
	Weapon->AnimS[1] = AnimS[1];
	Weapon->AnimS[2] = AnimS[2];
	Weapon->AnimS_Duration = amx_ctof(params[3]);
	Weapon->Delay = amx_ctof(params[4]);
	Weapon->Damage = amx_ctof(params[5]);
	Weapon->Recoil = amx_ctof(params[6]);
	return NULL;
}

cell AMX_NATIVE_CALL BuildWeaponReload(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	CWeapon *Weapon = &Weapons[Index];

	Weapon->AnimR = params[2];
	Weapon->AnimR_Duration = amx_ctof(params[3]);
	return NULL;
}

cell AMX_NATIVE_CALL BuildWeaponReload2(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	CWeapon *Weapon = &Weapons[Index];

	Weapon->AnimR_Duration = amx_ctof(params[2]);
	Weapon->AnimR = params[3];
	return NULL;
}

cell AMX_NATIVE_CALL BuildWeaponFlags(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	Weapons[Index].Flags = params[2];
	return NULL;
}

cell AMX_NATIVE_CALL BuildWeaponAttack2(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	CWeapon *Weapon = &Weapons[Index];
	Weapon->A2V = new A2V;
	//Weapon->A2V->TRV = NULL;

	switch (Weapon->A2I = params[2])
	{
		case A2_Zoom:
		{
			Weapon->A2V->WA2_ZOOM_MODE = *MF_GetAmxAddr(amx, params[3]);
			break;
		}
		case A2_Switch:
		{
			Weapon->A2V->WA2_SWITCH_ANIM_A = *MF_GetAmxAddr(amx, params[3]);
			Weapon->A2V->WA2_SWITCH_ANIM_A_DURATION = amx_ctof(*MF_GetAmxAddr(amx, params[4]));
			Weapon->A2V->WA2_SWITCH_ANIM_B = *MF_GetAmxAddr(amx, params[5]);
			Weapon->A2V->WA2_SWITCH_ANIM_B_DURATION = amx_ctof(*MF_GetAmxAddr(amx, params[6]));
			Weapon->A2V->WA2_SWITCH_ANIM_IDLE = *MF_GetAmxAddr(amx, params[7]);
			Weapon->A2V->WA2_SWITCH_ANIM_DRAW = *MF_GetAmxAddr(amx, params[8]);
			Weapon->A2V->WA2_SWITCH_ANIM_DRAW_DURATION = amx_ctof(*MF_GetAmxAddr(amx, params[9]));
			Weapon->A2V->WA2_SWITCH_ANIM_SHOOT = *MF_GetAmxAddr(amx, params[10]);
			Weapon->A2V->WA2_SWITCH_ANIM_SHOOT_DURATION = amx_ctof(*MF_GetAmxAddr(amx, params[11]));
			Weapon->A2V->WA2_SWITCH_ANIM_RELOAD = *MF_GetAmxAddr(amx, params[12]);
			Weapon->A2V->WA2_SWITCH_ANIM_RELOAD_DURATION = amx_ctof(*MF_GetAmxAddr(amx, params[13]));
			Weapon->A2V->WA2_SWITCH_DELAY = amx_ctof(*MF_GetAmxAddr(amx, params[14]));
			Weapon->A2V->WA2_SWITCH_DAMAGE = amx_ctof(*MF_GetAmxAddr(amx, params[15]));
			Weapon->A2V->WA2_SWITCH_RECOIL = amx_ctof(*MF_GetAmxAddr(amx, params[16]));
			PRECACHE_SOUND(Weapon->A2V->WA2_SWITCH_FSOUND = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[17], NULL, NULL))));
			break;
		}
		case A2_Burst:
		{
			Weapon->A2V->WA2_BURST_VALUE = *MF_GetAmxAddr(amx, params[3]);
			break;
		}
		case A2_MultiShot:
		{
			Weapon->A2V->WA2_MULTISHOT_VALUE = *MF_GetAmxAddr(amx, params[3]);
			break;
		}
		case A2_AutoPistol:
		{
			Weapon->A2V->WA2_AUTOPISTOL_ANIM = *MF_GetAmxAddr(amx, params[3]);
			Weapon->A2V->WA2_AUTOPISTOL_DELAY = amx_ctof(*MF_GetAmxAddr(amx, params[4]));
			Weapon->A2V->WA2_AUTOPISTOL_RECOIL = amx_ctof(*MF_GetAmxAddr(amx, params[5]));
			break;
		}
		case A2_KnifeAttack:
		{
			Weapon->A2V->WA2_KNIFEATTACK_ANIMATION = *MF_GetAmxAddr(amx, params[3]);
			Weapon->A2V->WA2_KNIFEATTACK_DELAY = amx_ctof(*MF_GetAmxAddr(amx, params[4]));
			Weapon->A2V->WA2_KNIFEATTACK_DURATION = amx_ctof(*MF_GetAmxAddr(amx, params[5]));
			Weapon->A2V->WA2_KNIFEATTACK_RADIUS = amx_ctof(*MF_GetAmxAddr(amx, params[6]));
			Weapon->A2V->WA2_KNIFEATTACK_DAMAGE_MIN = *MF_GetAmxAddr(amx, params[7]);
			Weapon->A2V->WA2_KNIFEATTACK_DAMAGE_MAX = *MF_GetAmxAddr(amx, params[8]);
			Weapon->A2V->WA2_KNIFEATTACK_KNOCKBACK = amx_ctof(*MF_GetAmxAddr(amx, params[9]));
			PRECACHE_SOUND(Weapon->A2V->WA2_KNIFEATTACK_SOUND = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[10], NULL, NULL))));
			break;
		}
		case A2_RadiusDamage:
		{

			break;
		}
		case A2_Charge:
		{
			break;
		}
		case A2_InstaSwitch:
		{
			Weapon->A2V->WA2_INSTASWITCH_ANIM_SHOOT = *MF_GetAmxAddr(amx, params[3]);
			Weapon->A2V->WA2_INSTASWITCH_DELAY = amx_ctof(*MF_GetAmxAddr(amx, params[4]));
			Weapon->A2V->WA2_INSTASWITCH_DAMAGE = amx_ctof(*MF_GetAmxAddr(amx, params[5]));
			Weapon->A2V->WA2_INSTASWITCH_RECOIL = amx_ctof(*MF_GetAmxAddr(amx, params[6]));
			Weapon->A2V->WA2_INSTASWITCH_NAME = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[7], NULL, NULL)));
			Weapon->A2V->WA2_INSTASWITCH_NAME2 = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[8], NULL, NULL)));
		}
	}

	return NULL;
}

cell AMX_NATIVE_CALL BuildWeaponMaxSpeed(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	Weapons[Index].Speed = amx_ctof(params[2]);
	return NULL;
}

cell AMX_NATIVE_CALL RegisterWeaponForward(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	CWeapon *Weapon = &Weapons[Index];
	char *String = MF_GetAmxString(amx, params[3], NULL, NULL);

	int Forward = params[2];

	if (Forward == WForward::DamagePost)
		Weapon->Forwards[Forward] = MF_RegisterSPForwardByName(amx, String, FP_CELL, FP_FLOAT, FP_DONE);
	else
		Weapon->Forwards[Forward] = MF_RegisterSPForwardByName(amx, String, FP_CELL, FP_DONE);

	return NULL;
}

cell AMX_NATIVE_CALL PrecacheWeaponModel(AMX *amx, cell *params)
{
	for (int Index = 1; Index <= params[0] / (signed)sizeof(cell); Index++)
		PRECACHE_MODEL(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[Index], NULL, NULL))));

	return NULL;
}

cell AMX_NATIVE_CALL PrecacheWeaponSound(AMX *amx, cell *params)
{
	for (int Index = 1; Index <= params[0] / (signed)sizeof(cell); Index++)
		PRECACHE_SOUND(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[Index], NULL, NULL))));

	return NULL;
}

cell AMX_NATIVE_CALL PrecacheWeaponGeneric(AMX *amx, cell *params)
{
	for (int Index = 1; Index <= params[0] / (signed)sizeof(cell); Index++)
		PRECACHE_GENERIC(STRING(ALLOC_STRING(MF_GetAmxString(amx, params[Index], NULL, NULL))));

	return NULL;
}

cell AMX_NATIVE_CALL FindWeaponByName(AMX *amx, cell *params)
{
	char *Name = MF_GetAmxString(amx, params[1], NULL, NULL);

	for (int Index = NULL; Index < WeaponCount; Index++)
	{
		if (!strcmp(Name, STRING(Weapons[Index].Model)))
			return Index;
	}

	return -1;
}

static cell AMX_NATIVE_CALL GiveWeaponByName(AMX *amx, cell *params)
{
	cell PIndex = params[1];

	if (PIndex < 0 || PIndex > gpGlobals->maxClients)
		return FALSE;

	GiveWeaponByName(INDEXENT(PIndex), (char *)MF_GetAmxString(amx, params[1], NULL, NULL));
	return TRUE;
}

static cell AMX_NATIVE_CALL GiveWeaponByID(AMX *amx, cell *params)
{
	cell PIndex = params[1];
	cell WIndex = params[2];

	if (PIndex < 0 || PIndex > gpGlobals->maxClients)
		return FALSE;

	if (WIndex < 0 || WIndex >= WeaponCount)
		return FALSE;

	GiveWeapon(INDEXENT(PIndex), WIndex);
	return TRUE;
}

static cell AMX_NATIVE_CALL GetWeaponData(AMX *amx, cell *params)
{
	edict_t *WeaponEdict = EDICT_FOR_NUM(params[1]);

	if (!WeaponEdict)
		return NULL;

	CBasePlayerWeapon *BaseWeapon;

	if (!(BaseWeapon = (CBasePlayerWeapon *)WeaponEdict->pvPrivateData))
		return NULL;

	switch (params[2])
	{
		case WData::IsCustom: return CUSTOM_WEAPON(BaseWeapon);
		case WData::Attack2: return WEAPON_A2(BaseWeapon);
		case WData::Attack2Offset: return WEAPON_A2_OFFSET(BaseWeapon);
		case WData::InAttack2: return WEAPON_INA2(BaseWeapon);
		case WData::InAttack2Delay: return WEAPON_INA2_DELAY(BaseWeapon);
		case WData::Flags: return WEAPON_A2(BaseWeapon);
	}

	return NULL;
}

static cell AMX_NATIVE_CALL SetWeaponData(AMX *amx, cell *params)
{
	edict_t *WeaponEdict = EDICT_FOR_NUM(params[1]);

	if (!WeaponEdict)
		return NULL;

	CBasePlayerWeapon *BaseWeapon;

	if (!(BaseWeapon = (CBasePlayerWeapon *)WeaponEdict->pvPrivateData))
		return NULL;

	int Value = params[3];

	switch (params[2])
	{
		//case WData::IsCustom: return CUSTOM_WEAPON(BaseWeapon) = Value; break;
		case WData::Attack2: return WEAPON_A2(BaseWeapon) = Value; break;
		case WData::Attack2Offset: return WEAPON_A2_OFFSET(BaseWeapon) = Value; break;
		case WData::InAttack2: return WEAPON_INA2(BaseWeapon) = Value; break;
		case WData::InAttack2Delay: return WEAPON_INA2_DELAY(BaseWeapon) = Value; break;
		case WData::Flags: return WEAPON_A2(BaseWeapon) = Value; break;
	}

	return NULL;
}

void CallSendWeaponAnim(CBasePlayerWeapon *BaseWeapon, int Anim);
static cell AMX_NATIVE_CALL SendWeaponAnim(AMX *amx, cell *params)
{
	edict_t *WeaponEdict = EDICT_FOR_NUM(params[1]);

	if (!WeaponEdict)
		return NULL;

	CBasePlayerWeapon *BaseWeapon;

	if (!(BaseWeapon = (CBasePlayerWeapon *)WeaponEdict->pvPrivateData))
		return NULL;

	CallSendWeaponAnim(BaseWeapon, params[2]);
	return NULL;
}

cell AMX_NATIVE_CALL CreateProjectile(AMX *amx, cell *params)
{
	CProjectile Projectile;
	PRECACHE_MODEL(Projectile.Model = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[1], NULL, NULL))));
	Projectile.Gravity = amx_ctof(params[2]);
	Projectile.Speed = amx_ctof(params[3]);
	Projectile.Forward = MF_RegisterSPForwardByName(amx, MF_GetAmxString(amx, params[4], NULL, NULL), FP_CELL, FP_DONE);

	if (params[0] / sizeof(cell) > 4)
		Projectile.Duration = amx_ctof(*MF_GetAmxAddr(amx, params[5]));

	Projectiles.append(Projectile);

	ProjectileCount++;
	return ProjectileCount - 1;
}

cell ShootProjectileTimed(edict_t *LauncherEdict, int ProjectileID);
static cell AMX_NATIVE_CALL _ShootProjectileTimed(AMX *amx, cell *params)
{
	return ShootProjectileTimed(EDICT_FOR_NUM(params[1]), params[2]);
}

cell ShootProjectileContact(edict_t *LauncherEdict, int ProjectileID);
static cell AMX_NATIVE_CALL _ShootProjectileContact(AMX *amx, cell *params)
{
	return ShootProjectileContact(EDICT_FOR_NUM(params[1]), params[2]);
}

int Player_GiveAmmoByID(CBasePlayer *BasePlayer, int AmmoID, int Amount);
static cell AMX_NATIVE_CALL GiveAmmo(AMX *amx, cell *params)
{
	edict_t *PlayerEdict = EDICT_FOR_NUM(params[1]);

	if (InvalidEntity(PlayerEdict), IS_USER_DEAD(PlayerEdict))
		return NULL;

	Player_GiveAmmoByID((CBasePlayer *)PlayerEdict->pvPrivateData, params[2], params[3]);
	return NULL;
}

cell AMX_NATIVE_CALL SetAmmoName(AMX *amx, cell *params)
{
	int AmmoIndex = params[1];

	if (AmmoIndex < 0 || AmmoIndex >= AmmoCount)
		return NULL;

	Ammos[AmmoIndex].Name = STRING(ALLOC_STRING(MF_GetAmxString(amx, params[2], NULL, NULL)));
	return NULL;
}

cell AMX_NATIVE_CALL FindAmmoByName(AMX *amx, cell *params)
{
	char *SearchAmmoName = MF_GetAmxString(amx, params[1], NULL, NULL);
	const char *AmmoName;

	for (unsigned int Index = NULL; Index < Ammos.length(); Index++)
	{
		AmmoName = Ammos[Index].Name;

		if (params[2])
		{
			if (!strcmp(SearchAmmoName, AmmoName))
				return (cell)Index;
		}
		else
		{
			if (!stricmp(SearchAmmoName, AmmoName))
				return (cell)Index;
		}
	}

	return NULL;
}

cell AMX_NATIVE_CALL GetWeaponCount(AMX *amx, cell *params)
{
	return WeaponCount;
}

cell AMX_NATIVE_CALL GetWeaponName(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	MF_SetAmxString(amx, params[2], Weapons[Index].Name, params[3]);
	return NULL;
}

cell AMX_NATIVE_CALL SetWeaponPathAddOn(AMX *amx, cell *params)
{
	strcpy_s(PathAddOn, MF_GetAmxString(amx, params[1], NULL, NULL));
	return NULL;
}

cell AMX_NATIVE_CALL GetWeaponFlags(AMX *amx, cell *params)
{
	cell Index = params[1];

	if (Index < 0 || Index >= WeaponCount)
		return NULL;

	return Weapons[Index].Flags;
}

void PlayerKnockback(edict_t *VictimEdict, Vector Origin, float Knockback);
static cell AMX_NATIVE_CALL RadiusDamage2(AMX *amx, cell *params)
{
	cell *VectorPtr = MF_GetAmxAddr(amx, params[1]);
	Vector Origin = { amx_ctof(VectorPtr[0]), amx_ctof(VectorPtr[1]), amx_ctof(VectorPtr[2]) };

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(Origin[0]);
	WRITE_COORD(Origin[1]);
	WRITE_COORD(Origin[2] + 40.f);
	WRITE_SHORT(MI_Explosion);
	WRITE_BYTE(25);
	WRITE_BYTE(25);
	WRITE_BYTE(0);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_WORLDDECAL);
	WRITE_COORD(Origin[0]);
	WRITE_COORD(Origin[1]);
	WRITE_COORD(Origin[2]);
	WRITE_BYTE(RANDOM_LONG(46, 48));
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_SMOKE);
	WRITE_COORD(Origin[0]);
	WRITE_COORD(Origin[1]);
	WRITE_COORD(Origin[2]);
	WRITE_SHORT(MI_Smoke);
	WRITE_BYTE(30);
	WRITE_BYTE(10);
	MESSAGE_END();

	edict_t *TargetEdict = SVGame_Edicts;
	edict_t *ProjectileEdict = EDICT_FOR_NUM(params[4]);
	edict_t *OwnerEdict = ProjectileEdict->v.owner;
	float Radius = amx_ctof(params[2]);
	entvars_t *AttackerEntVars;
	int CastTeam;

	if (InvalidEntity(OwnerEdict))
	{
		AttackerEntVars = &ProjectileEdict->v;
		CastTeam = AttackerEntVars->team;
	}
	else
	{
		AttackerEntVars = &OwnerEdict->v;
		CastTeam = *((int *)OwnerEdict->pvPrivateData + 114);
	}
	
	float Damage = amx_ctof(params[3]);
	CBaseEntity *BaseEntity;
	int Flags = params[5];
	TraceResult TR;
	
	while ((TargetEdict = FIND_ENTITY_IN_SPHERE(TargetEdict, Origin, Radius)) != SVGame_Edicts)
	{
		if (!TargetEdict->pvPrivateData)
			continue;

		if (!TargetEdict->v.takedamage)
			continue;

		if (!(Flags & PFlag::Penetration))
		{
			TRACE_LINE(Origin, TargetEdict->v.origin, 0, 0, &TR);

			if (TR.pHit != TargetEdict)
				continue;
		}

		if ((TargetEdict == OwnerEdict) && (Flags & PFlag::NoSelfDamage))
			continue;

		BaseEntity = (CBaseEntity *)TargetEdict->pvPrivateData;
		BaseEntity->TakeDamage(AttackerEntVars, AttackerEntVars, (TargetEdict == OwnerEdict ? 0.4 : 1.0) * ((Damage * Radius) / (TargetEdict->v.origin - Origin).Length()), DMG_SLASH/*DMG_EXPLOSION*/);
		
		if ((Flags & PFlag::Knockback) && BaseEntity->IsPlayer())
		{
			if (!MP_FriendlyFire && (CastTeam == *((int *)BaseEntity + 114)))
				continue;

			PlayerKnockback(TargetEdict, Origin, RANDOM_FLOAT(120.f, 150.f));
		}
	}

	return NULL;
}

static cell AMX_NATIVE_CALL CanPrimaryAttack(AMX *amx, cell *params)
{
	CBasePlayerWeapon *BaseWeapon = (CBasePlayerWeapon *)(EDICT_FOR_NUM(params[1])->pvPrivateData);
	return ((BaseWeapon->m_flNextPrimaryAttack <= 0.f) && (BaseWeapon->m_pPlayer->m_flNextAttack <= 0.f)) ? TRUE : FALSE;
}

static cell AMX_NATIVE_CALL SetNextAttack(AMX *amx, cell *params)
{
	CBasePlayerWeapon *BaseWeapon = (CBasePlayerWeapon *)(EDICT_FOR_NUM(params[1])->pvPrivateData);
	BaseWeapon->m_flNextPrimaryAttack = BaseWeapon->m_flNextSecondaryAttack = BaseWeapon->m_flTimeWeaponIdle = amx_ctof(params[2]);
	return NULL;
}

static cell AMX_NATIVE_CALL SetCustomIdleAnim(AMX *amx, cell *params)
{
	((CBasePlayerWeapon *)EDICT_FOR_NUM(params[1])->pvPrivateData)->m_iFamasShotsFired = params[2];
	return NULL;
}

cell AMX_NATIVE_CALL GetWeaponPath(AMX *amx, cell *params)
{
	char Path[64];
	Q_sprintf(Path, "models/%s/%s", PathAddOn, Weapons[params[1]].Model);
	MF_SetAmxString(amx, params[2], Path, params[3]);
	return NULL;
}

static cell AMX_NATIVE_CALL SetPlayerViewModel(AMX *amx, cell *params)
{
	EDICT_FOR_NUM(params[1])->v.viewmodel = (string_t)params[2];
	return NULL;
}

static cell AMX_NATIVE_CALL SetPlayerWeapModel(AMX *amx, cell *params)
{
	EDICT_FOR_NUM(params[1])->v.weaponmodel = (string_t)params[2];
	return NULL;
}

cell AMX_NATIVE_CALL PrecacheWeaponModel2(AMX *amx, cell *params)
{
	string_t Model = ALLOC_STRING(MF_GetAmxString(amx, params[1], NULL, nullptr));
	PRECACHE_MODEL(STRING(Model));
	return (cell)Model;
}

static int GetSequenceFlags(void *pmodel, entvars_t *pev)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;

	if (!pstudiohdr || pev->sequence >= pstudiohdr->numseq)
	{
		return 0;
	}

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + int(pev->sequence);
	return pseqdesc->flags;
}

static int GetSequenceFlags(CBasePlayer *BasePlayer)
{
	void *pmodel = GET_MODEL_PTR(ENT(BasePlayer->pev));
	return ::GetSequenceFlags(pmodel, BasePlayer->pev);
}

static void GetSequenceInfo(void *pmodel, entvars_t *pev, float *pflFrameRate, float *pflGroundSpeed)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;

	if (!pstudiohdr)
	{
		return;
	}

	if (pev->sequence >= pstudiohdr->numseq)
	{
		*pflFrameRate = 0;
		*pflGroundSpeed = 0;
		return;
	}

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + int(pev->sequence);
	if (pseqdesc->numframes <= 1)
	{
		*pflFrameRate = 256.0f;
		*pflGroundSpeed = 0.0f;
		return;
	}

	*pflFrameRate = pseqdesc->fps * 256.0f / (pseqdesc->numframes - 1);
	*pflGroundSpeed = Q_sqrt(pseqdesc->linearmovement[0] * pseqdesc->linearmovement[0] + pseqdesc->linearmovement[1] * pseqdesc->linearmovement[1] + pseqdesc->linearmovement[2] * pseqdesc->linearmovement[2]);
	*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
}

static void ResetSequenceInfo(CBasePlayer *BasePlayer)
{
	void *Model = GET_MODEL_PTR(ENT(BasePlayer->pev));

	GetSequenceInfo(Model, BasePlayer->pev, &BasePlayer->m_flFrameRate, &BasePlayer->m_flGroundSpeed);
	BasePlayer->m_fSequenceLoops = ((GetSequenceFlags(BasePlayer) & STUDIO_LOOPING) != 0);
	BasePlayer->pev->animtime = gpGlobals->time;
	//BasePlayer->pev->framerate = 1.0f;
	BasePlayer->m_fSequenceFinished = FALSE;
	BasePlayer->m_flLastEventCheck = gpGlobals->time;
}

int LookupActivity(void *pmodel, entvars_t *pev, int activity)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;

	if (!pstudiohdr)
	{
		return 0;
	}

	mstudioseqdesc_t *pseqdesc;

	int i;
	int weightTotal = 0;
	int activitySequenceCount = 0;
	int weight = 0;
	int select;

	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

	for (i = 0; i < pstudiohdr->numseq; i++)
	{
		if (pseqdesc[i].activity == activity)
		{
			weightTotal += pseqdesc[i].actweight;
			activitySequenceCount++;
		}
	}

	if (activitySequenceCount > 0)
	{
		if (weightTotal)
		{
			int which = RANDOM_LONG(0, weightTotal - 1);

			for (i = 0; i < pstudiohdr->numseq; i++)
			{
				if (pseqdesc[i].activity == activity)
				{
					weight += pseqdesc[i].actweight;

					if (weight > which)
					{
						return i;
					}
				}
			}
		}
		else
		{
			select = RANDOM_LONG(0, activitySequenceCount - 1);

			for (i = 0; i < pstudiohdr->numseq; i++)
			{
				if (pseqdesc[i].activity == activity)
				{
					if (select == 0)
					{
						return i;
					}

					select--;
				}
			}
		}
	}

	return ACT_INVALID;
}

int LookupActivity(CBasePlayer *BasePlayer, int activity)
{
	void *pmodel = GET_MODEL_PTR(ENT(BasePlayer->pev));
	return ::LookupActivity(pmodel, BasePlayer->pev, activity);
}

void SetAnimation(edict_t *PlayerEdict, int Animation, Activity IActivity, float FrameRate)
{
	entvars_t *PlayerEntVars = &PlayerEdict->v;
	CBasePlayer *BasePlayer = (CBasePlayer *)PlayerEdict->pvPrivateData;

	if (IActivity == ACT_SWIM || IActivity == ACT_LEAP || IActivity == ACT_HOVER)
		return;

	BasePlayer->m_flLastFired = gpGlobals->time;

	PlayerEntVars->frame = 0.f;
	PlayerEntVars->framerate = FrameRate;
	PlayerEntVars->sequence = Animation;
	PlayerEntVars->animtime = gpGlobals->time;

	//BasePlayer->m_fSequenceLoops = NULL;
	//BasePlayer->m_fSequenceFinished = FALSE;
	BasePlayer->m_Activity = IActivity;
	BasePlayer->m_IdealActivity = IActivity;

	BasePlayer->m_flGroundSpeed = 0.f;
	BasePlayer->m_flFrameRate = FrameRate;
	BasePlayer->m_flLastEventCheck = gpGlobals->time;
	ResetSequenceInfo(BasePlayer);
}

static cell AMX_NATIVE_CALL _SetAnimation(AMX *amx, cell *params)
{
	edict_t *PlayerEdict = EDICT_FOR_NUM(params[1]);

	if (!PlayerEdict->pvPrivateData)
		return NULL;

	SetAnimation(PlayerEdict, params[2], (Activity)params[3], amx_ctof(params[4]));
	return NULL;
}

cell AMX_NATIVE_CALL GetWeaponDefaultDelay(AMX *amx, cell *params)
{
	return amx_ftoc(WEAPON_DEFAULT_DELAY[params[1]]);
}

AMX_NATIVE_INFO AMXX_NATIVES[] =
{
	{ "CreateWeapon", CreateWeapon },
	{ "CreateAmmo", CreateAmmo },
	{ "BuildWeaponFireSound", BuildWeaponFireSound },
	{ "BuildWeaponAmmunition", BuildWeaponAmmunition },
	{ "BuildWeaponDeploy", BuildWeaponDeploy },
	{ "BuildWeaponShoot", BuildWeaponShoot },
	{ "BuildWeaponReload", BuildWeaponReload },
	{ "BuildWeaponReload2", BuildWeaponReload2 },
	{ "BuildWeaponFlags", BuildWeaponFlags },
	{ "BuildWeaponAttack2", BuildWeaponAttack2 },
	{ "BuildWeaponMaxSpeed", BuildWeaponMaxSpeed },
	{ "RegisterWeaponForward", RegisterWeaponForward },
	{ "PrecacheWeaponModel", PrecacheWeaponModel },
	{ "PrecacheWeaponSound", PrecacheWeaponSound },
	{ "PrecacheWeaponGeneric", PrecacheWeaponGeneric },
	{ "FindWeaponByName", FindWeaponByName },
	{ "GiveWeaponByName", GiveWeaponByName },
	{ "GiveWeaponByID", GiveWeaponByID },
	{ "GetWeaponData", GetWeaponData },
	{ "SetWeaponData", SetWeaponData },
	{ "SendWeaponAnim", SendWeaponAnim },
	{ "CreateProjectile", CreateProjectile },
	{ "ShootProjectileTimed", _ShootProjectileTimed },
	{ "ShootProjectileContact", _ShootProjectileContact },
	{ "GiveAmmo", GiveAmmo },
	{ "SetAmmoName", SetAmmoName },
	{ "FindAmmoByName", FindAmmoByName },
	{ "GetWeaponCount", GetWeaponCount },
	{ "GetWeaponName", GetWeaponName },
	{ "SetWeaponPathAddOn", SetWeaponPathAddOn },
	{ "GetWeaponFlags", GetWeaponFlags },
	{ "RadiusDamage2", RadiusDamage2 },
	{ "CanPrimaryAttack", CanPrimaryAttack },
	{ "SetNextAttack", SetNextAttack },
	{ "SetCustomIdleAnim", SetCustomIdleAnim },
	{ "GetWeaponPath", GetWeaponPath },
	{ "SetPlayerViewModel", SetPlayerViewModel },
	{ "SetPlayerWeapModel", SetPlayerWeapModel },
	{ "PrecacheWeaponModel2", PrecacheWeaponModel2 },
	{ "SetAnimation", _SetAnimation },
	{ "GetWeaponDefaultDelay", GetWeaponDefaultDelay },
	{ NULL, NULL }
};

void OnAmxxAttach(void)
{
	MF_AddNatives(AMXX_NATIVES);
}

