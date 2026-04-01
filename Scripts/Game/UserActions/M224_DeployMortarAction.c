/*!
 * M224_DeployMortarAction.c
 *
 * Two user actions:
 *
 *   M224_DeployMortarAction  – on the carried M224 weapon entity.
 *     Shown when the player has the M224 in their hands and is standing
 *     on sufficiently flat ground.  Spawns the deployed mortar entity at
 *     the player's feet and removes the carried weapon from the world.
 *
 *   M224_PickUpMortarAction  – on the deployed M224_Mortar_Deployed entity.
 *     Shown when a player is close enough to the deployed mortar.
 *     Removes the deployed entity and puts a fresh carried M224 into the
 *     player's inventory.
 */

// ============================================================
//  M224_DeployMortarAction
// ============================================================

class M224_DeployMortarAction : ScriptedUserAction
{
	//! Prefab to spawn as the deployed (emplaced) mortar.
	[Attribute("{00000000000000000000000000000000}", UIWidgets.ResourcePickerThumbnail,
		"Deployed mortar prefab (M224_Mortar_Deployed.et)", "et")]
	protected ResourceName m_sDeployedPrefab;

	//! Maximum terrain tilt (degrees) that still allows deployment.
	[Attribute("20", UIWidgets.Slider, "Max terrain slope for deployment (degrees)", "0 45 1")]
	protected float m_fMaxSlopeDeg;

	// ──────────────────────────── Visibility / permission ────────────────────────────

	override bool CanBeShownScript(IEntity user)
	{
		return ChimeraCharacter.Cast(user) != null;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (m_sDeployedPrefab.IsEmpty())
		{
			SetCannotPerformReason("No deployed prefab configured");
			return false;
		}

		ChimeraCharacter character = ChimeraCharacter.Cast(user);
		if (!character)
			return false;

		// ── Ground slope check ──────────────────────────────────────────────
		vector charPos = character.GetOrigin();

		TraceParam trace = new TraceParam();
		trace.Start = charPos + "0 0.5 0";
		trace.End   = charPos - "0 3.0 0";
		trace.Flags = TraceFlags.WORLD;

		float frac = GetGame().GetWorld().TraceMove(trace, null);
		if (frac >= 1.0)
		{
			SetCannotPerformReason("No ground below");
			return false;
		}

		// Dot of surface normal with world-up gives cos(slope angle).
		float cosAngle = vector.Dot(trace.TraceNorm, "0 1 0");
		float slopeDeg = Math.Acos(Math.Clamp(cosAngle, -1.0, 1.0)) * Math.RAD2DEG;
		if (slopeDeg > m_fMaxSlopeDeg)
		{
			SetCannotPerformReason("Ground too steep");
			return false;
		}

		return true;
	}

	// ──────────────────────────── Execution (server) ────────────────────────────

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;

		// ── Build spawn transform at player feet, facing same direction ──
		vector transform[4];
		character.GetTransform(transform);

		// Snap Y down to the surface.
		vector traceStart = transform[3] + "0 0.5 0";
		vector traceEnd   = transform[3] - "0 3.0 0";

		TraceParam trace = new TraceParam();
		trace.Start = traceStart;
		trace.End   = traceEnd;
		trace.Flags = TraceFlags.WORLD;

		float frac = GetGame().GetWorld().TraceMove(trace, null);
		if (frac < 1.0)
			transform[3] = traceStart + (traceEnd - traceStart) * frac;

		// ── Spawn deployed mortar ──────────────────────────────────────────
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform     = transform;

		IEntity deployedMortar = GetGame().SpawnEntityPrefab(
			Resource.Load(m_sDeployedPrefab), GetGame().GetWorld(), spawnParams);

		if (!deployedMortar)
		{
			Print("[M224] FAILED to spawn deployed mortar prefab: " + m_sDeployedPrefab, LogLevel.ERROR);
			return;
		}

		// ── Delete the carried weapon entity ──────────────────────────────
		RplComponent.DeleteRplEntity(pOwnerEntity, false);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Deploy M224 Mortar";
		return true;
	}

	override bool CanBroadcastScript() { return true; }
}

// ============================================================
//  M224_PickUpMortarAction
// ============================================================

class M224_PickUpMortarAction : ScriptedUserAction
{
	//! Prefab for the carried M224 weapon that is returned to inventory.
	[Attribute("{00000000000000000000000000000000}", UIWidgets.ResourcePickerThumbnail,
		"Carried mortar prefab (M224_Mortar.et)", "et")]
	protected ResourceName m_sCarriedPrefab;

	// ──────────────────────────── Visibility / permission ────────────────────────────

	override bool CanBeShownScript(IEntity user)
	{
		return ChimeraCharacter.Cast(user) != null;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		if (m_sCarriedPrefab.IsEmpty())
		{
			SetCannotPerformReason("No carried prefab configured");
			return false;
		}

		// Prevent pick-up while someone is actively operating the mortar.
		SCR_MortarMuzzleComponent muzzle = SCR_MortarMuzzleComponent.Cast(
			GetOwner().FindComponent(SCR_MortarMuzzleComponent));
		if (muzzle && muzzle.IsBeingLoaded())
		{
			SetCannotPerformReason("Mortar is being loaded");
			return false;
		}

		return true;
	}

	// ──────────────────────────── Execution (server) ────────────────────────────

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;

		// ── Spawn a fresh carried M224 and insert into inventory ──────────
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		character.GetTransform(spawnParams.Transform);

		IEntity newM224 = GetGame().SpawnEntityPrefab(
			Resource.Load(m_sCarriedPrefab), GetGame().GetWorld(), spawnParams);

		if (newM224)
		{
			SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(
				character.FindComponent(SCR_InventoryStorageManagerComponent));

			if (invMgr)
				invMgr.TryInsertItemInStorage(newM224, invMgr.GetCharacterStorage());
			else
				Print("[M224] Could not find inventory manager on character – M224 dropped at origin", LogLevel.WARNING);
		}
		else
		{
			Print("[M224] FAILED to spawn carried M224 prefab: " + m_sCarriedPrefab, LogLevel.ERROR);
		}

		// ── Remove the deployed mortar entity ─────────────────────────────
		RplComponent.DeleteRplEntity(pOwnerEntity, false);
	}

	override bool GetActionNameScript(out string outName)
	{
		outName = "Pick Up M224 Mortar";
		return true;
	}

	override bool CanBroadcastScript() { return true; }
}
