/*!
 * M224_ShellAutoPickupComponent.c
 *
 * Attached to the deployed M224_Mortar_Deployed entity.
 * After each shot fires, searches the operator's inventory for the
 * next shell matching the selected type and raises it into the
 * character's hand so they can immediately drop it into the barrel.
 *
 * Also contains a modded extension of SCR_MortarMuzzleComponent that
 * exposes an invoker for the weapon-fired and shell-loaded events, and
 * stores the last loader character reference.
 */

// ============================================================
//  modded SCR_MortarMuzzleComponent
//  Adds a fire-event invoker and captures the loader character.
// ============================================================

modded class SCR_MortarMuzzleComponent
{
	//! Fired after every mortar shot.  Params: (lastLoader).
	protected ref ScriptInvoker<ChimeraCharacter> m_OnM224WeaponFired;

	//! Returns (or lazily creates) the weapon-fired invoker.
	ScriptInvoker<ChimeraCharacter> GetOnM224WeaponFired()
	{
		if (!m_OnM224WeaponFired)
			m_OnM224WeaponFired = new ScriptInvoker<ChimeraCharacter>();
		return m_OnM224WeaponFired;
	}

	//! The character that last called LoadShell on this muzzle.
	protected ChimeraCharacter m_M224_LastLoader;

	//! Override LoadShell to capture the loader character.
	override void LoadShell(notnull SCR_MortarShellGadgetComponent shellComp, notnull ChimeraCharacter character, float fuzeTime = -1, bool fromLeftSide = false)
	{
		m_M224_LastLoader = character;
		super.LoadShell(shellComp, character, fuzeTime, fromLeftSide);
	}

	//! Override OnWeaponFired to invoke our custom invoker.
	override void OnWeaponFired(IEntity effectEntity, BaseMuzzleComponent muzzle, IEntity projectileEntity)
	{
		super.OnWeaponFired(effectEntity, muzzle, projectileEntity);
		if (m_OnM224WeaponFired)
			m_OnM224WeaponFired.Invoke(m_M224_LastLoader);
	}
}

// ============================================================
//  M224_ShellAutoPickupComponent
// ============================================================

[ComponentEditorProps(category: "M224 Mortar", description: "Auto-equips the next matching shell from the operator's inventory after each shot.")]
class M224_ShellAutoPickupComponentClass : ScriptGameComponentClass {};

//------------------------------------------------------------------------------------------------
class M224_ShellAutoPickupComponent : ScriptGameComponent
{
	protected SCR_MortarMuzzleComponent m_MuzzleComp;
	protected M224_RoundSelectorComponent m_SelectorComp;

	// ──────────────────────────── Lifecycle ────────────────────────────

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		m_MuzzleComp   = SCR_MortarMuzzleComponent.Cast(owner.FindComponent(SCR_MortarMuzzleComponent));
		m_SelectorComp = M224_RoundSelectorComponent.Cast(owner.FindComponent(M224_RoundSelectorComponent));

		if (!m_MuzzleComp)
		{
			Print("[M224_ShellAutoPickup] SCR_MortarMuzzleComponent not found on deployed mortar!", LogLevel.WARNING);
			return;
		}

		m_MuzzleComp.GetOnM224WeaponFired().Insert(OnWeaponFired);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_MuzzleComp)
			m_MuzzleComp.GetOnM224WeaponFired().Remove(OnWeaponFired);
	}

	// ──────────────────────────── Fire callback ────────────────────────────

	protected void OnWeaponFired(ChimeraCharacter loader)
	{
		if (!loader)
			return;

		M224_EShellType desiredType = M224_EShellType.HE;
		if (m_SelectorComp)
			desiredType = m_SelectorComp.GetSelectedType();

		IEntity nextShell = FindShellInInventory(loader, desiredType);
		if (nextShell)
			EquipShellInHand(loader, nextShell);
		else
			SendOutOfAmmoHint(loader, desiredType);
	}

	// ──────────────────────────── Inventory search ────────────────────────────

	protected IEntity FindShellInInventory(notnull ChimeraCharacter character, M224_EShellType desiredType)
	{
		SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
			character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!invMgr)
			return null;

		array<IEntity> allItems = {};
		invMgr.GetAllRootItems(allItems);

		foreach (IEntity item : allItems)
		{
			SCR_MortarShellGadgetComponent shellGadget = SCR_MortarShellGadgetComponent.Cast(
				item.FindComponent(SCR_MortarShellGadgetComponent));
			if (!shellGadget)
				continue;

			M224_ShellTypeComponent typeComp = M224_ShellTypeComponent.Cast(
				item.FindComponent(M224_ShellTypeComponent));
			if (!typeComp)
				continue;

			if (typeComp.GetShellType() == desiredType)
				return item;
		}

		return null;
	}

	// ──────────────────────────── Gadget equip ────────────────────────────

	protected void EquipShellInHand(notnull ChimeraCharacter character, notnull IEntity shellEntity)
	{
		SCR_GadgetManagerComponent gadgetMgr = SCR_GadgetManagerComponent.GetGadgetManager(character);
		if (!gadgetMgr)
			return;

		gadgetMgr.SetGadgetMode(shellEntity, EGadgetMode.IN_HAND);
	}

	// ──────────────────────────── Notifications ────────────────────────────

	protected void SendOutOfAmmoHint(notnull ChimeraCharacter character, M224_EShellType type)
	{
		string typeName;
		switch (type)
		{
			case M224_EShellType.HE:            typeName = "HE";                break;
			case M224_EShellType.HighLethality: typeName = "High Lethality HE"; break;
			case M224_EShellType.SmokeRed:      typeName = "Smoke (Red)";       break;
			case M224_EShellType.SmokeWhite:    typeName = "Smoke (White)";     break;
			case M224_EShellType.WPSmoke:       typeName = "WP Smoke";          break;
		}

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
		if (playerId <= 0)
			return;

		// TODO: Replace with a proper "Out of <type>" notification or HUD hint.
		Print("[M224] Out of " + typeName + " for player " + playerId, LogLevel.DEBUG);
	}
}
