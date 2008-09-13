-- The Forgotten Server Config

	-- Account Manager
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

	-- Connection Config
	ip = "127.0.0.1"
	port = 7171
	loginTries = 5
	retryTimeout = 30 * 1000
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

	-- Guilds
	inGameGuildManagement = "yes"
	levelToFormGuild = 8
	guildNameMinLength = 4
	guildNameMaxLength = 20

	-- Highscores and Deathlist
	highscoreDisplayPlayers = 15
	updateHighscoresAfterMinutes = 60
	deathListEnabled = "yes"
	maxDeathRecords = 5

	-- Houses
	buyableAndSellableHouses = "yes"
	housesPerAccount = 0
	levelToBuyHouse = 1
	houseRentAsPrice = "no"
	housePriceAsRent = "no"
	housePriceEachSQM = 1000
	houseRentPeriod = "never"
	houseNeedPremiumAccount = "yes"	

	-- Premium
	freePremium = "no"
	premiumForPromotion = "yes"
	removePremiumOnInit = "yes"

	-- Summons
	maxPlayerSummons = 2
	teleportAllSummons = "no" --FIXME: doesn't work
	teleportPlayerSummons = "no" --FIXME: doesn't work

	-- Miscellaneous
	allowChangeOutfit = "yes"
	kickIdlePlayerAfterMinutes = 15
	maxMessageBuffer = 4
	displayGamemastersWithOnlineCommand = "no"
	defaultPriority = "high"
	bankSystem = "yes"
	displaySkillLevelOnAdvance = "no" --FIXME: causes crash in some cases

	-- Item Usage
	timeBetweenActions = 200
	timeBetweenExActions = 1000

	-- Map
	mapName = "forgotten"
	mapAuthor = "Komic"
	randomizeTiles = "yes"
	cleanProtectedZones = "yes"
	
	-- Database
	passwordType = "plain"
	sqlType = "sqlite"
	sqliteDatabase = "forgottenserver.s3db"
	mysqlHost = "localhost"
	mysqlUser = "root"
	mysqlPass = ""
	mysqlDatabase = "theforgottenserver"
	mysqlPort = 3306

	-- Rates
	rateExp = 5
	rateSkill = 3
	rateLoot = 2
	rateMagic = 3
	rateSpawn = 1

	-- Server Save
	serverSaveEnabled = "yes"
	serverSaveHour = 3
	shutdownAtServerSave = "yes"
	cleanMapAtServerSave = "yes"

	-- Spawns
	deSpawnRange = 2
	deSpawnRadius = 50

	-- Status
	ownerName = ""
	ownerEmail = "@otland.net"
	url = "http://otland.net/"
	location = "Europe"