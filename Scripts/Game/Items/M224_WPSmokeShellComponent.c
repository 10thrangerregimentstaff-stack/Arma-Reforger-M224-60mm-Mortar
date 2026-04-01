/*!
 * M224_WPSmokeShellComponent.c
 *
 * Component placed on the WP Smoke shell prefab (M224_Shell_WPSmoke.et).
 * Hooks into the shell's detonation/used event and spawns a long-duration
 * WP smoke particle effect.
 *
 * The particle effect itself must be configured in Workbench with:
 *   - Emitter "Occluder Type" set to "Smoke" for thermal / IR blocking.
 *   - Duration ~60 seconds.
 *
 * NOTE: This component should be added alongside SCR_MortarShellGadgetComponent
 *       on the WP shell gadget prefab, NOT on the projectile prefab.
 *       It subscribes to GetOnShellUsed() which fires when the shell is launched.
 */

[ComponentEditorProps(category: "M224 Mortar", description: "WP smoke effect for M224 WP shell – spawns thermal-blocking smoke on detonation.")]
class M224_WPSmokeShellComponentClass : ScriptGameComponentClass {};

//------------------------------------------------------------------------------------------------
class M224_WPSmokeShellComponent : ScriptGameComponent
{
	//! Particle effect resource for WP smoke (configure in Workbench; needs Smoke occluder emitters).
	[Attribute("", UIWidgets.ResourcePickerThumbnail,
		"WP smoke particle effect resource (.ptc). Must have Occluder Type = Smoke emitters.", "ptc")]
	protected ResourceName m_sWPSmokeParticle;

	//! Duration (seconds) that the WP smoke persists.
	[Attribute("60", UIWidgets.Slider, "WP smoke duration in seconds", "5 300 5")]
	protected float m_fSmokeDurationSec;

	protected SCR_MortarShellGadgetComponent m_ShellGadgetComp;

	// ──────────────────────────── Lifecycle ────────────────────────────

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_ShellGadgetComp = SCR_MortarShellGadgetComponent.Cast(
			owner.FindComponent(SCR_MortarShellGadgetComponent));

		if (!m_ShellGadgetComp)
		{
			Print("[M224_WPSmoke] SCR_MortarShellGadgetComponent missing from WP shell entity!", LogLevel.WARNING);
			return;
		}

		// Subscribe to the shell-used invoker fired when the shell is launched.
		m_ShellGadgetComp.GetOnShellUsed().Insert(OnShellUsed);
	}

	override void OnDelete(IEntity owner)
	{
		if (m_ShellGadgetComp)
			m_ShellGadgetComp.GetOnShellUsed().Remove(OnShellUsed);
	}

	// ──────────────────────────── Shell-used callback ────────────────────────────

	//! Called when the mortar fires and this shell is the launched round.
	//! Server-side: schedules a WP smoke spawn at the impact point via the projectile.
	//! Because we only have the gadget here (not the projectile), we rely on the
	//! projectile prefab also having a ParticleEffectEntityComponent configured with
	//! the WP particle on impact.  This method is a hook for any extra server logic
	//! (e.g. environment damage) that needs to run once the shell fires.
	protected void OnShellUsed()
	{
		// The actual in-world smoke is spawned by the projectile on impact.
		// (Configure the projectile prefab's effect component in Workbench.)
		//
		// If you need server-side logic at fire time (e.g. start a timer,
		// play a sound), add it here.
		Print("[M224] WP smoke shell fired.", LogLevel.DEBUG);
	}

	// ──────────────────────────── Static helper ────────────────────────────

	/*!
	 * Spawns a WP smoke entity at the given world position (for use in the
	 * projectile's impact script, if you choose to call this from there).
	 *
	 * Usage from the WP projectile on impact:
	 *   M224_WPSmokeShellComponent.SpawnWPSmokeAtPosition(impactPos, smokeParticlePath, duration);
	 */
	static void SpawnWPSmokeAtPosition(vector worldPos, ResourceName particlePrefab, float durationSec = 60.0)
	{
		if (!Replication.IsServer())
			return;

		if (particlePrefab.IsEmpty())
		{
			Print("[M224_WPSmoke] No particle prefab set – skipping WP smoke spawn.", LogLevel.WARNING);
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = worldPos;

		IEntity smokeEnt = GetGame().SpawnEntityPrefab(
			Resource.Load(particlePrefab), GetGame().GetWorld(), spawnParams);

		if (!smokeEnt)
		{
			Print("[M224_WPSmoke] Failed to spawn WP smoke particle entity!", LogLevel.ERROR);
			return;
		}

		// Schedule auto-delete after the smoke duration.
		GetGame().GetCallqueue().CallLater(DeleteSmokeEntity, durationSec * 1000, false, smokeEnt);
	}

	//! Deletes the smoke entity once its duration elapses.
	static void DeleteSmokeEntity(IEntity smokeEnt)
	{
		if (!smokeEnt)
			return;

		RplComponent.DeleteRplEntity(smokeEnt, false);
	}
}
