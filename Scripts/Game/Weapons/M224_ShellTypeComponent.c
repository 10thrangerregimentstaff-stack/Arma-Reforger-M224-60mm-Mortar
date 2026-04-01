/*!
 * M224_ShellTypeComponent.c
 *
 * Identifies the type of a mortar shell.  Place this component on every
 * M224 shell prefab and set m_eShellType to the correct value.
 * Used by M224_RoundSelectorComponent (selection) and
 * M224_ShellAutoPickupComponent (inventory search).
 */

//! All five round types carried by the M224.
enum M224_EShellType
{
	HE			= 0,	//!< High Explosive
	HighLethality	= 1,	//!< High-Lethality HE
	SmokeRed	= 2,	//!< Red Smoke
	SmokeWhite	= 3,	//!< White Smoke
	WPSmoke		= 4		//!< White Phosphorus Smoke (IR/thermal blocking)
}

[ComponentEditorProps(category: "M224 Mortar", description: "Identifies the mortar shell type for inventory matching.")]
class M224_ShellTypeComponentClass : ScriptGameComponentClass {};

//------------------------------------------------------------------------------------------------
class M224_ShellTypeComponent : ScriptGameComponent
{
	[Attribute("0", UIWidgets.ComboBox, "Shell type for this prefab", "", ParamEnumArray.FromEnum(M224_EShellType))]
	protected M224_EShellType m_eShellType;

	//! Returns the shell type assigned to this prefab.
	M224_EShellType GetShellType()
	{
		return m_eShellType;
	}
}
