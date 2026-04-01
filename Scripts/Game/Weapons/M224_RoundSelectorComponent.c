/*!
 * M224_RoundSelectorComponent.c
 *
 * Attached to the deployed M224_Mortar_Deployed entity.
 * Lets the current operator choose which round type to load next via
 * five keybinds (bound in the game's key bindings menu).
 *
 * Input actions that must be registered in the mod's input config:
 *   M224_SelectHE, M224_SelectHL, M224_SelectSmokeRed,
 *   M224_SelectSmokeWhite, M224_SelectWPSmoke
 */

[ComponentEditorProps(category: "M224 Mortar", description: "Lets the player select which M224 round type to load next.")]
class M224_RoundSelectorComponentClass : ScriptGameComponentClass {};

//------------------------------------------------------------------------------------------------
class M224_RoundSelectorComponent : ScriptGameComponent
{
	//! Currently selected shell type (server-authoritative, replicated).
	[RplProp(onRplName: "OnSelectedTypeReplicated")]
	protected int m_iSelectedType;	// M224_EShellType cast to int for RplProp

	// ──────────────────────────── API ────────────────────────────

	//! Returns the currently selected shell type.
	M224_EShellType GetSelectedType()
	{
		return m_iSelectedType;
	}

	// ──────────────────────────── Lifecycle ────────────────────────────

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Input listeners are client-side only.
		if (!GetGame().GetPlayerController())
			return;

		GetGame().GetInputManager().AddActionListener("M224_SelectHE",         EActionTrigger.DOWN, OnSelectHE);
		GetGame().GetInputManager().AddActionListener("M224_SelectHL",         EActionTrigger.DOWN, OnSelectHL);
		GetGame().GetInputManager().AddActionListener("M224_SelectSmokeRed",   EActionTrigger.DOWN, OnSelectSmokeRed);
		GetGame().GetInputManager().AddActionListener("M224_SelectSmokeWhite", EActionTrigger.DOWN, OnSelectSmokeWhite);
		GetGame().GetInputManager().AddActionListener("M224_SelectWPSmoke",    EActionTrigger.DOWN, OnSelectWPSmoke);
	}

	override void OnDelete(IEntity owner)
	{
		if (!GetGame().GetPlayerController())
			return;

		GetGame().GetInputManager().RemoveActionListener("M224_SelectHE",         EActionTrigger.DOWN, OnSelectHE);
		GetGame().GetInputManager().RemoveActionListener("M224_SelectHL",         EActionTrigger.DOWN, OnSelectHL);
		GetGame().GetInputManager().RemoveActionListener("M224_SelectSmokeRed",   EActionTrigger.DOWN, OnSelectSmokeRed);
		GetGame().GetInputManager().RemoveActionListener("M224_SelectSmokeWhite", EActionTrigger.DOWN, OnSelectSmokeWhite);
		GetGame().GetInputManager().RemoveActionListener("M224_SelectWPSmoke",    EActionTrigger.DOWN, OnSelectWPSmoke);
	}

	// ──────────────────────────── Input callbacks ────────────────────────────

	protected void OnSelectHE()         { RequestSelectType(M224_EShellType.HE); }
	protected void OnSelectHL()         { RequestSelectType(M224_EShellType.HighLethality); }
	protected void OnSelectSmokeRed()   { RequestSelectType(M224_EShellType.SmokeRed); }
	protected void OnSelectSmokeWhite() { RequestSelectType(M224_EShellType.SmokeWhite); }
	protected void OnSelectWPSmoke()    { RequestSelectType(M224_EShellType.WPSmoke); }

	//! Sends the selection to the server (or sets directly if already on server).
	protected void RequestSelectType(M224_EShellType type)
	{
		Rpc(RpcAsk_SetSelectedType, type);
	}

	// ──────────────────────────── RPC ────────────────────────────

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_SetSelectedType(M224_EShellType type)
	{
		m_iSelectedType = type;
		// Broadcast the new value to all clients via RplProp.
		Replication.BumpMe();

		ShowSelectionHint(type);
	}

	// ──────────────────────────── Replication ────────────────────────────

	//! Called on every client when m_iSelectedType is replicated.
	protected void OnSelectedTypeReplicated()
	{
		ShowSelectionHint(m_iSelectedType);
	}

	// ──────────────────────────── HUD hint ────────────────────────────

	//! Shows a short on-screen hint for the local player, if available.
	protected void ShowSelectionHint(M224_EShellType type)
	{
		// Only show on the local client.
		if (!GetGame().GetPlayerController())
			return;

		string typeName;
		switch (type)
		{
			case M224_EShellType.HE:           typeName = "HE";           break;
			case M224_EShellType.HighLethality: typeName = "High Lethality HE"; break;
			case M224_EShellType.SmokeRed:     typeName = "Smoke (Red)";  break;
			case M224_EShellType.SmokeWhite:   typeName = "Smoke (White)"; break;
			case M224_EShellType.WPSmoke:      typeName = "WP Smoke";     break;
		}

		SCR_HintManagerComponent hintMgr = SCR_HintManagerComponent.GetInstance();
		if (hintMgr)
			hintMgr.ShowCustomHint("M224: " + typeName + " selected", "", 2.0);
	}
}
