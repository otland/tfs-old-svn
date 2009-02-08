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

	-- Battle
	-- NOTE: loginProtectionPeriod is the famous Tibia anti-magebomb system.
	worldType = "pvp"
	hotkeyAimbotEnabled = "yes"
	protectionLevel = 1
	pvpTileIgnoreLevelAndVocationProtection = "yes"
	killsToRedSkull = 3
	pzLocked = 60 * 1000
	criticalHitChance = 7
	displayCriticalHitNotify = "no"
	removeWeaponAmmunition = "yes"
	removeWeaponCharges = "yes"
	removeRuneCharges = "yes"
	timeToDecreaseFrags = 24 * 60 * 60 * 1000
	whiteSkullTime = 15 * 60 * 1000
	noDamageToSameLookfeet = "no"
	experienceByKillingPlayers = "no"
	showHealingDamage = "no"
	fieldOwnershipDuration = 5 * 1000
	stopAttackingAtExit = "no"
	oldConditionAccuracy = "no"
	loginProtectionPeriod = 10

	-- Connection config
	worldId = 0
	ip = "127.0.0.1"
	port = 7171
	loginTries = 10
	retryTimeout = 5 * 1000
	loginTimeout = 60 * 1000
	maxPlayers = "1000"
	motd = "Welcome to the Forgotten Server!"
	displayOnOrOffAtCharlist = "no"
	onePlayerOnlinePerAccount = "yes"
	allowClones = 0
	serverName = "Forgotten"
	loginMessage = "Welcome to the Forgotten Server!"
	adminLogsEnabled = "no"
	statusTimeout = 5 * 60 * 1000
	replaceKickOnLogin = "yes"
	forceSlowConnectionsToDisconnect = "no"
	loginOnlyWithLoginServer = "no"

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
	sqlKeepAlive = 60
	mysqlReadTimeout = 3
	optimizeDatabaseAtStartup = "yes"
	passwordType = "plain"

	-- Deathlist
	deathListEnabled = "yes"
	maxDeathRecords = 5

	-- Guilds
	ingameGuildManagement = "yes"
	levelToFormGuild = 8
	guildNameMinLength = 4
	guildNameMaxLength = 20

	-- Highscores
	highscoreDisplayPlayers = 15
	updateHighscoresAfterMinutes = 60

	-- Houses
	buyableAndSellableHouses = "yes"
	housesPerAccount = 0
	levelToBuyHouse = 1
	houseRentAsPrice = "no"
	housePriceAsRent = "no"
	housePriceEachSQM = 1000
	houseRentPeriod = "never"

	-- Item usage
	timeBetweenActions = 200
	timeBetweenExActions = 1000

	-- Map
	-- NOTE: storeTrash costs more memory, but will perform alot faster cleaning.
	-- useHouseDataStorage usage may be found at README.
	mapName = "forgotten"
	mapAuthor = "Komic"
	randomizeTiles = "yes"
	useHouseDataStorage = "no"
	storeTrash = "yes"
	cleanProtectedZones = "yes"

	-- Miscellaneous
	-- NOTE: defaultPriority works only on Windows
	-- promptExceptionTracerErrorBox works only with precompiled support feature,
	-- called "exception tracer" (__EXCEPTION_TRACER__ flag).
	defaultPriority = "high"
	maxMessageBuffer = 4
	kickIdlePlayerAfterMinutes = 15
	allowChangeOutfit = "yes"
	allowChangeColors = "yes"
	disableOutfitsForPrivilegedPlayers = "no"
	displayGamemastersWithOnlineCommand = "no"
	bankSystem = "yes"
	saveGlobalStorage = "yes"
	displaySkillLevelOnAdvance = "no"
	spellNameInsteadOfWords = "no"
	emoteSpells = "no"
	expireReportsAfterReads = 1
	promptExceptionTracerErrorBox = "yes"

	-- Premium account
	freePremium = "no"
	removePremiumOnInit = "yes"
	premiumForPromotion = "yes"
	blessingsOnlyPremium = "yes"
	houseNeedPremium = "yes"
	bedsRequirePremium = "yes"

	-- Rates
	-- NOTE: experienceStages configuration is located in data/XML/stages.xml.
	rateExperience = 5.0
	rateSkill = 3.0
	rateMagic = 3.0
	rateLoot = 2
	rateSpawn = 1
	experienceStages = "no"

	-- Party
	-- NOTE experienceShareLevelDifference is float number.
	-- 0.66666666666667 is highestLevel * 2 / 3
	experienceShareRadiusX = 30
	experienceShareRadiusY = 30
	experienceShareRadiusZ = 1
	experienceShareLevelDifference = 0.66666666666667
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

	-- Logs
	-- NOTE: This kind of logging does not work in GUI version.
	-- For such, please compile the software with __GUI_LOGS__ flag.
	outLogName = "server/out.log"
	errorLogName = "server/error.log"
	truncateLogsOnStartup = "yes"
