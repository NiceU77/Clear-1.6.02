/******************************************************************************
 *
 * mayabase API functions.
 *
 *****************************************************************************/

/**
 * \file
 * \brief mayabase API Interface (Public)
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MBRP_BEAUTY = 0,
	MBRP_COLOR = 1,
	MBRP_SHADOW = 2,
	MBRP_AMBIENT = 3,
	MBRP_DIFFUSE = 4,
	MBRP_SPECULAR = 5,
	MBRP_GLOBILLUM = 6
} MbRenderPass;

typedef enum {
	MBSL_IGNORE = 0,
	MBSL_OBEYS_LIGHTS = 1,
	MBSL_OBEYS_SHADOW = 2
} MbShadowLinking;

typedef enum {
	MBSI_NULL = 0,
	/* Options. */
	MBSI_RENDERPASS = 1,
	MBSI_SHADOWLIMIT = 2,
	MBSI_SHADOWLINKING = 3, /* Obsolete */
	MBSI_COMPUTEFILTERSIZE = 4,
	MBSI_DEFAULTFILTERSIZE = 5,
	MBSI_REFRACTIONBLURLIMIT = 30,
	MBSI_REFLECTIONBLURLIMIT = 31,
	MBSI_AGGRESSIVECACHING = 6,
	MBSI_GLOWCOLORBUFFER  = 33,
	MBSI_CUSTOMALPHA = 34,
	MBSI_CUSTOMLABEL = 35,
	MBSI_CUSTOMDEPTH = 36, /* Largest */
	/* Frame Constants. */
	MBSI_XPIXELANGLE = 7,
	/* Light Data Array. */
	MBSI_LIGHTAMBIENT = 8,
	MBSI_LIGHTDIFFUSE = 9,
	MBSI_LIGHTSPECULAR = 10,
	MBSI_SHADOWFRACTION = 11,
	MBSI_PRESHADOWCOLOR = 12,
	MBSI_NORMALINVERTED = 13,
	MBSI_CANDROPSHADOW = 32,
	/* Particle State. */
	MBSI_PARTICLE_ID = 14,
	MBSI_PARTICLE_AGE = 15,
	MBSI_PARTICLE_LIFESPAN = 16,
	MBSI_PARTICLE_WEIGHT = 17,
	MBSI_PARTICLE_COLOR = 18,
	MBSI_PARTICLE_TRANSPARENCY = 19,
	MBSI_PARTICLE_INCANDESCENCE = 20,
	MBSI_PARTICLE_EMISSION = 21,
	MBSI_PARTICLE_POINTWORLD = 22,
	MBSI_PARTICLE_FARPOINTWORLD = 23,
	/* Main State Items. */
	MBSI_OPAQUE = 24,
	MBSI_ACCGLOWCOLOR = 25,
	MBSI_ACCMATTEOPACITY = 26,
	MBSI_ACCTRANSPARENCY = 27,
	MBSI_UVFILTERSIZE = 28,
	MBSI_FILTERSIZE = 29
} MbStateItem;

DLLEXPORT
miBoolean
mayabase_stateitem_get(
	miState		*state,
	...);

DLLEXPORT
miBoolean
mayabase_lightlink_get(
	miTag		lightLink,
	int		*numLights,
	miTag		**lights,
	miState		*state);

DLLEXPORT
miBoolean
mayabase_lightlink_check(
	miTag		light,
	int		numLights,
	miTag		*lights,
	miState		*state);

DLLEXPORT
miBoolean
mayabase_texcoords_check(
	const miVector	*texCoord,
	int		textureDimension,
	miBoolean	wrap);

DLLEXPORT
miBoolean
mayabase_parm_connection(
	const miState	*state,
	const void	*parm);

#ifdef __cplusplus
}
#endif
