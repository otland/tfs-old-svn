-- The Forgotten Server Config

	-- Account manager
	accountManager = "yes"
	namelockManager = "yes"
	newPlayerChooseVoc = "no"
	newPlayerSpawnPosX = 95
	newPlayerSpawnPosY = 117
	newPlayerSpawnPosZ = 7
	newPlayerTownId = 1
	newPlayerLevel = 1
	newPlayerMagicLevel = 0
	generateAccountNumber = "no"

	-- Banishments
	notationsToBan = 3
	warningsToFinalBan = 4
	warningsToDeletion = 5
	banLength = 7 * 24 * 60 * 60
	finalBanLength = 30 * 24 * 60 * 60
	ipBanishmentLength = 1 * 24 * 60 * 60
	broadcastBanishments = "yes"
	killsToBan = 5
	maxViolationCommentSize = 200
	autoBanishUnknownBytes = "no"

	-- Battle
	-- NOTE: loginProtectionPeriod is the famous Tibia anti-magebomb system.
	-- deathLostPercent set to nil enables manual mode.
	-- showHealingDamageForMonsters inheritates from showHealingDamage.
	worldType = "pvp"
	protectionLevel = 1
	pvpTileIgnoreLevelAndVocationProtection = "yes"
	killsToRedSkull = 3
	pzLocked = 60 * 1000
	criticalHitChance = 7
	criticalHitMultiplier = 1
	displayCriticalHitNotify = "no"
	removeWeaponAmmunition = "yes"
	removeWeaponCharges = "yes"
	removeRuneCharges = "yes"
	timeToDecreaseFrags = 24 * 60 * 60 * 1000
	whiteSkullTime = 15 * 60 * 1000
	noDamageToSameLookfeet = "no"
	experienceByKillingPlayers = "no"
	showHealingDamage = "no"
	showHealingDamageForMonsters = "no"
	fieldOwnershipDuration = 5 * 1000
	stopAttackingAtExit = "no"
	oldConditionAccuracy = "no"
	loginProtectionPeriod = 10 * 1000
	deathLostPercent = 10
	stairhopDelay = 2 * 1000
	pushCreatureDelay = 2 * 1000
	deathContainerId = 1987
	gainExperienceColor = 215

	-- Connection config
	worldId = 0
	ip = "127.0.0.1"
	loginPort = 7171
	gamePort = 7172
	adminPort = 7171
	statusPort = 7171
	loginTries = 10
	retryTimeout = 5 * 1000
	loginTimeout = 60 * 1000
	maxPlayers = 1000
	motd = "Welcome to the Forgotten Server!"
	displayOnOrOffAtCharlist = "no"
	onePlayerOnlinePerAccount = "yes"
	allowClones = 0
	serverName = "Forgotten"
	loginMessage = "Welcome to the Forgotten Server!"
	statusTimeout = 5 * 60 * 1000
	replaceKickOnLogin = "yes"
	forceSlowConnectionsToDisconnect = "no"
	loginOnlyWithLoginServer = "no"
	premiumPlayerSkipWaitList = "no"

	-- Database
	-- NOTE: sqlFile is used only by sqlite database, and sqlKeepAlive by mysql database.
	-- To disable sqlKeepAlive such as mysqlReadTimeout use 0 value.
	sqlType = "sqlite"
	sqlHost = "localhost"
	sqlPort = 3306
	sqlUser = "root"
	sqlPass = ""
	sqlDatabase = "theforgottenserver"
	sqlFile = "forgottenserver.s3db"
	sqlKeepAlive = 0
	mysqlReadTimeout = 10
	mysqlWriteTImeout = 10
	passwordType = "plain"

	-- Deathlist
	deathListEnabled = "yes"
	maxDeathRecords = 5

	-- Guilds
	ingameGuildManagement = "yes"
	levelToFormGuild = 8
	premiumDaysToFormGuild = 0
	guildNameMinLength = 4
	guildNameMaxLength = 20

	-- Highscores
	highscoreDisplayPlayers = 15
	updateHighscoresAfterMinutes = 60

	-- Houses
	buyableAndSellableHouses = "yes"
	houseNeedPremium = "yes"
	bedsRequirePremium = "yes"
	levelToBuyHouse = 1
	housesPerAccount = 0
	houseRentAsPrice = "no"
	housePriceAsRent = "no"
	housePriceEachSquare = 1000
	houseRentPeriod = "never"

	-- Item usage
	timeBetweenActions = 200
	timeBetweenExActions = 1000
	checkCorpseOwner = "yes"
	hotkeyAimbotEnabled = "yes"
	maximumDoorLevel = 500

	-- Map
	-- NOTE: storeTrash costs more memory, but will perform alot faster cleaning.
	-- useHouseDataStorage usage may be found at README.
	mapName = "forgotten"
	mapAuthor = "Komic"
	randomizeTiles = "yes"
	useHouseDataStorage = "no"
	storeTrash = "yes"
	cleanProtectedZones = "yes"

	-- Startup
	-- NOTE: defaultPriority works only on Windows and niceLevel on *nix
	-- coresUsed are seperated by comma cores ids used by server process,
	-- default is -1, so it stays untouched (automaticaly assigned by OS).
	defaultPriority = "high"
	niceLevel = 5
	coresUsed = "-1"
	optimizeDatabaseAtStartup = "yes"
	removePremiumOnInit = "yes"
	confirmOutdatedVersion = "yes"

	-- Muted buffer
	maxMessageBuffer = 4
	bufferMutedOnSpellFailure = "no"

	-- Miscellaneous
	-- NOTE: promptExceptionTracerErrorBox works only with precompiled support feature,
	-- called "exception tracer" (__EXCEPTION_TRACER__ flag).
	dataDirectory = "data/"
	kickIdlePlayerAfterMinutes = 15
	allowChangeOutfit = "yes"
	allowChangeColors = "yes"
	allowChangeAddons = "yes"
	disableOutfitsForPrivilegedPlayers = "no"
	bankSystem = "yes"
	saveGlobalStorage = "yes"
	ghostModeInvisibleEffect = "no"
	displaySkillLevelOnAdvance = "no"
	spellNameInsteadOfWords = "no"
	emoteSpells = "no"
	expireReportsAfterReads = 1
	promptExceptionTracerErrorBox = "yes"
	storePlayerDirection = "no"
	playerQueryDeepness = 2

	-- Premium-related
	freePremium = "no"
	premiumForPromotion = "yes"

	-- Blessings
	-- NOTE: blessingReduction* regards items/containers loss.
	-- eachBlessReduction is how much each bless reduces the experience/magic/skills loss.
	blessingsOnlyPremium = "yes"
	blessingReductionBase = 30
	blessingReductionDecreament = 5
	eachBlessReduction = 8

	-- Rates
	-- NOTE: experienceStages configuration is located in data/XML/stages.xml.
	experienceStages = "no"
	rateExperience = 5.0
	rateSkill = 3.0
	rateMagic = 3.0
	rateLoot = 2.0
	rateSpawn = 1

	-- Stamina
	-- NOTE: Stamina is stored in miliseconds, so seconds are multiplied by 1000.
	-- rateStaminaHits multiplies every hit done a creature, which are later
	-- multiplied by player attack speed.
	-- rateStaminaGain is multiplying every second of logged out time, eg:
	-- 60 * 1000 / 3 = 20 seconds, what gives 1 stamina minute for 3 being logged off.
	-- rateStaminaThresholdGain is dividing in case the normal gain (that is
	-- multiplied by rateStaminaGain, btw.) passed above threshold, eg:
	-- 60 * 1000 / 3 = 20 / 4 = 5 seconds (3 * 4 = 12 minutes for 1 stamina minute).
	-- staminaRatingLimit* is in minutes.
	rateStaminaLoss = 1
	rateStaminaGain = 1000 / 3
	rateStaminaThresholdGain = 4
	staminaRatingLimitTop = 41 * 60
	staminaRatingLimitBottom = 14 * 60
	rateStaminaAboveNormal = 1.5
	rateStaminaUnderNormal = 0.5
	staminaThresholdOnlyPremium = "yes"

	-- Party
	-- NOTE: experienceShareLevelDifference is float number.
	-- experienceShareLevelDifference is highestLevel * value
	experienceShareRadiusX = 30
	experienceShareRadiusY = 30
	experienceShareRadiusZ = 1
	experienceShareLevelDifference = 2 / 3
	extraPartyExperienceLimit = 20
	extraPartyExperiencePercent = 5

	-- Global save
	-- NOTE: globalSaveHour means like 03:00, not that it will save every 3 hours,
	-- if you want such a system please check out data/globalevents/globalevents.xml.
	globalSaveEnabled = "no"
	globalSaveHour = 8
	shutdownAtGlobalSave = "yes"
	cleanMapAtGlobalSave = "no"

	-- Spawns
	deSpawnRange = 2
	deSpawnRadius = 50

	-- Summons
	maxPlayerSummons = 2
	teleportAllSummons = "no"
	teleportPlayerSummons = "no"

	-- Status
	ownerName = ""
	ownerEmail = "@otland.net"
	url = "http://otland.net/"
	location = "Europe"
	displayGamemastersWithOnlineCommand = "no"

	-- Logs
	-- NOTE: This kind of logging does not work in GUI version.
	-- For such, please compile the software with __GUI_LOGS__ flag.
	adminLogsEnabled = "no"
	displayPlayersLogging = "yes"
	prefixChannelLogs = ""
	runeFile = ""
	outLogName = ""
	errorLogName = ""
	truncateLogsOnStartup = "no"
