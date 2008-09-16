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
	generateAccountNumber = "yes"

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

	-- Connection config
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

	-- Database
	sqlType = "sqlite"
	sqlHost = "localhost"
	sqlPort = 3306
	sqlUser = "root"
	sqlPass = ""
	sqlDatabase = "theforgottenserver"
	sqlFile "forgottenserver.s3db"
	-- NOTE: sqlFile is used only by sqlite database
	passwordType = "plain"

	-- Deathlist
	deathListEnabled = "yes"
	maxDeathRecords = 5

	-- Guilds
	inGameGuildManagement = "yes"
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
	houseNeedPremiumAccount = "yes"	

	-- Item usage
	timeBetweenActions = 200
	timeBetweenExActions = 1000

	-- Map
	mapName = "forgotten"
	mapAuthor = "Komic"
	randomizeTiles = "yes"
	cleanProtectedZones = "yes"

	-- Miscellaneous
	kickIdlePlayerAfterMinutes = 15
	maxMessageBuffer = 4
	displayGamemastersWithOnlineCommand = "no"
	defaultPriority = "high"
	bankSystem = "yes"
	displaySkillLevelOnAdvance = "no" --FIXME: Causes crash in some cases
	spellNameInsteadOfWordsOnCast = "no" --FIXME: Causes crashes
	saveGlobalStorage = "yes"

	-- Premium account
	freePremium = "no"
	premiumForPromotion = "yes"
	removePremiumOnInit = "yes"

	-- Rates
	rateExp = 5
	rateSkill = 3
	rateLoot = 2
	rateMagic = 3
	rateSpawn = 1

	-- Global save
	-- NOTE: serverSaveHour means like 03:00, not that it will save every 3 hours,
	-- if you want such a system use autoSaveEachMinutes. this serversave method
	-- may be unstable, we recommend using otadmin if you want real serversaves.
	globalSaveEnabled = "no"
	globalSaveHour = 3
	shutdownAtGlobalSave = "yes"
	cleanMapAtGlobalSave = "yes"

	-- Server save
	autoSaveEachMinutes = 15
	saveGlobalStorage = "no"

	-- Spawns
	deSpawnRange = 2
	deSpawnRadius = 50

	-- Summons
	maxPlayerSummons = 2
	teleportAllSummons = "no" --FIXME: doesn't work
	teleportPlayerSummons = "no" --FIXME: doesn't work

	-- Startup
	-- NOTE: This works only in Windows
	defaultPriority = "high"

	-- Status
	ownerName = ""
	ownerEmail = "@otland.net"
	url = "http://otland.net/"
	location = "Europe"