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
	worldType = "pvp"
	hotkeyAimbotEnabled = "yes"
	protectionLevel = 1
	pvpTileIgnoreLevelAndVocationProtection = "yes"
	killsToRedSkull = 3
	pzLocked = 60 * 1000
	criticalHitChance = 7
	displayCriticalHitNotify = "no"
	removeAmmoWhenUsingDistanceWeapon = "yes"
	removeChargesFromRunes = "yes"
	timeToDecreaseFrags = 24 * 60 * 60 * 1000
	whiteSkullTime = 15 * 60 * 1000
	noDamageToSameLookfeet = "no"
	experienceByKillingPlayers = "no"
	showHealingDamage = "no"
	fieldOwnershipDuration = 5 * 1000
	stopAttackingAtExit = "no"
	oldConditionAccuracy = "no"

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
	-- To disable sqlKeepAlive use 0 value.
	sqlType = "sqlite"
	sqlHost = "localhost"
	sqlPort = 3306
	sqlUser = "root"
	sqlPass = ""
	sqlDatabase = "theforgottenserver"
	sqlFile = "forgottenserver.s3db"
	sqlKeepAlive = 60
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
	mapName = "forgotten"
	mapAuthor = "Komic"
	randomizeTiles = "yes"
	cleanProtectedZones = "yes"

	-- Miscellaneous
	-- NOTE: defaultPriority works only on Windows
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

	-- Premium account
	freePremium = "no"
	removePremiumOnInit = "yes"
	premiumForPromotion = "yes"
	blessingsOnlyPremium = "yes"
	houseNeedPremium = "yes"
	bedsRequirePremium = "yes"

	-- Rates
	-- NOTE: experienceStages configuration is located in data/XML/stages.xml.
	rateExp = 5
	rateSkill = 3
	rateLoot = 2
	rateMagic = 3
	rateSpawn = 1
	extraPartyExpLimit = 20
	extraPartyExpPercent = 5
	experienceStages = "no"

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
	teleportAllSummons = "no" --FIXME: Doesn't work
	teleportPlayerSummons = "no" --FIXME: Doesn't work

	-- Status
	ownerName = ""
	ownerEmail = "@otland.net"
	url = "http://otland.net/"
	location = "Europe"
