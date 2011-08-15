--[[
	The 'CHANCE' values that I'm not sure about have been defaulted to 40%
	TODO: Get real FAIL_MSG and SUCCESS_MSG for some of the tammings
	
	Info:
		[xxxxx] 	 = tamming itemid
		NAME 		 = name of the creature tamming item is used on
		ID 			 = mount id for storage (see mounts.xml)
		TYPE 		 = type of creature taming item is used on (monster/npc)
		CHANCE 		 = X/100%
		FAIL_MSG 	 = {run_msg, break_msg, nothing_msg)
		SUCCESS_MSG  = "xxxxx"
]]

local config = {
	[5907] = 	{NAME = 'Bear', 				ID = 3, 	TYPE = "monster", 	CHANCE = 20, 	FAIL_MSG = {"The bear ran away.", "Oh no! The slingshot broke.", "The bear is trying to hit you with its claws."}, SUCCESS_MSG = "You tamed the wild bear."},
	[13295] = 	{NAME = 'Black Sheep', 			ID = 4, 	TYPE = "monster", 	CHANCE = 25, 	FAIL_MSG = {"Oh no! The reins were torn.", "The black sheep is trying to run away.", "The black sheep ran away."}, SUCCESS_MSG = "You tamed the black sheep."},
	[13293] = 	{NAME = 'Midnight Panther', 	ID = 5, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The panther has escaped.", "The whip broke."}, SUCCESS_MSG = "You tamed the panther."},
	[13298] = 	{NAME = 'Terror Bird', 			ID = 2, 	TYPE = "monster", 	CHANCE = 14, 	FAIL_MSG = {"The bird ran away.", "The terror bird is pecking you."}, SUCCESS_MSG = "You tamed the bird."},
	[13247] = 	{NAME = 'Boar', 				ID = 10, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The boar has run away", "The boar attacks you."}, SUCCESS_MSG = "You tamed the boar."},
	[13305] = 	{NAME = 'Crustacea Gigantica', 	ID = 7, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The crustacea has run away.", "The crustacea ate the shrimp."}, SUCCESS_MSG = "You have tamed the crustacea."},
	[13291] = 	{NAME = 'Undead Cavebear', 		ID = 12, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The undead bear has run away."}, SUCCESS_MSG = "You have tamed the skeleton."},
	[13307] = 	{NAME = 'Wailing Widow', 		ID = 1, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The widow has run away.", "The widow has eaten the sweet bait."}, SUCCESS_MSG = "You tamed the widow."},
	[13292] = 	{NAME = 'Tin Lizzard', 			ID = 8, 	TYPE = "npc", 		CHANCE = 40, 	FAIL_MSG = {"The key broke inside."}, SUCCESS_MSG = "You have started the Tin Lizzard!"},
	[13294] = 	{NAME = 'Draptor', 				ID = 6, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The draptor has fled.", "The draptor has run away."}, SUCCESS_MSG = "You tamed the draptor."},
	[13536] = 	{NAME = 'Crystal Wolf', 		ID = 16, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The wolf has run away."}, SUCCESS_MSG = "You tamed the wolf."},
	[13539] = 	{NAME = 'White Deer', 			ID = 18, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The deer has fled in fear.", "The cone broke."}, SUCCESS_MSG = "You tamed the deer."},
	[13538] = 	{NAME = 'Panda', 				ID = 19, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"Panda ate the leaves and ran away."}, SUCCESS_MSG = "You tamed the Panda."},
	[13535] = 	{NAME = 'Dromedary', 			ID = 20, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"Dromedary has run away."}, SUCCESS_MSG = "You have tamed Dromedary."},
	[13498] = 	{NAME = 'Sandstone Scorpion', 	ID = 21, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The scorpion has vanished.", "Scorpion broken the sceptre."}, SUCCESS_MSG = "You tamed the Scorpion"},
	[13537] = 	{NAME = 'Donkey', 				ID = 13, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The witch has esacped!"}, SUCCESS_MSG = "You tamed the Mule."},
	[13938] = 	{NAME = 'Uniwheel', 			ID = 15, 	TYPE = "npc", 		CHANCE = 40, 	FAIL_MSG = {"This Uniwheel the oil is having no effect."}, SUCCESS_MSG = "You found a Uniwheel."},
	[13508] = 	{NAME = 'Slug', 				ID = 14, 	TYPE = "monster", 	CHANCE = 40, 	FAIL_MSG = {"The slug has run away.", "The drug had no effect."}, SUCCESS_MSG = "You tamed the slug."},
	[13939] = 	{NAME = 'Fire Horse', 			ID = 23, 	TYPE = "monster", 	CHANCE = 15, 	FAIL_MSG = {"The horse runs away.", "The horse ate the oats."}, SUCCESS_MSG = "You tamed the War Horse."}
}

local actions = {
	"run",
	"break",
	"nothing"
}

local function doFailAction(cid, id, mount, pos, item, itemEx)
	action = actions[id]
	if(action == "run") then
		doSendMagicEffect(pos, CONST_ME_POFF)
		doRemoveCreature(itemEx.uid)
		doCreatureSay(cid, mount.FAIL_MSG[id], TALKTYPE_ORANGE_1)
	elseif(action == "break") then
		doSendMagicEffect(pos, CONST_ME_BLOCKHIT)
		doRemoveItem(item.uid, 1)
		doCreatureSay(cid, mount.FAIL_MSG[id], TALKTYPE_ORANGE_1)
	elseif(action == "nothing") then
		doSendMagicEffect(pos, CONST_ME_POFF)
		doCreatureSay(cid, mount.FAIL_MSG[id], TALKTYPE_ORANGE_1)
	end
	return action
end

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local mount = config[item.itemid]
	if(mount == nil or getPlayerMount(cid, mount.ID)) then
		return false
	end
	
	actionId, rand = math.random(1, #mount.FAIL_MSG), math.random(1, 100)
	--Monster Mount
	if(isMonster(itemEx.uid) and not isSummon(itemEx.uid) and mount.TYPE == "monster") then
		if(mount.NAME == getCreatureName(itemEx.uid)) then
			if(rand > mount.CHANCE) then
				doFailAction(cid, actionId, mount, toPosition, item, itemEx)
				return true
			else
				doPlayerAddMount(cid, mount.ID)
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_ORANGE, mount.SUCCESS_MSG)
				doCreatureSay(cid, mount.SUCCESS_MSG, TALKTYPE_ORANGE_1)
				doRemoveCreature(itemEx.uid)
				doSendMagicEffect(toPosition, CONST_ME_POFF)
				doRemoveItem(item.uid, 1)
				return true
			end
		end
	
	--NPC Mount
	elseif(isNpc(itemEx.uid) and not isMonster(itemEx.uid) and mount.TYPE == "npc") then
		if(mount.NAME == getCreatureName(itemEx.uid)) then
			if(rand > mount.CHANCE) then
				doFailAction(cid, actionId, mount, toPosition, item, itemEx)
				return true
			else
				doPlayerAddMount(cid, mount.ID)
				doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_ORANGE, mount.SUCCESS_MSG)
				doCreatureSay(cid, mount.SUCCESS_MSG, TALKTYPE_ORANGE_1)
				doSendMagicEffect(toPosition, CONST_ME_MAGIC_GREEN)
				doRemoveItem(item.uid, 1)
				return true
			end
		end
	end
	
	return false
end