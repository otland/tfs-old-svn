-- Advanced NPC System (Created by Jiddo),
-- Modified by Talaturen.
-- Modified by Elf.

if(Modules == nil) then
	-- default words for greeting and ungreeting the npc. Should be a table containing all such words.
	FOCUS_GREETWORDS = {'hi', 'hello', 'hey'}
	FOCUS_FAREWELLWORDS = {'bye', 'farewell', 'cya'}

	-- The words for requesting trade window.
	SHOP_TRADEREQUEST = {'offer', 'trade'}

	-- The word for accepting/declining an offer. CAN ONLY CONTAIN ONE FIELD! Should be a teble with a single string value.
	SHOP_YESWORD = {'yes'}
	SHOP_NOWORD = {'no'}

	-- Pattern used to get the amount of an item a player wants to buy/sell.
	PATTERN_COUNT = '%d+'

	-- Constants used to separate buying from selling.
	SHOPMODULE_SELL_ITEM = 1
	SHOPMODULE_BUY_ITEM = 2
	SHOPMODULE_BUY_ITEM_CONTAINER = 3

	-- Constants used for shop mode. Notice: addBuyableItemContainer is working on all modes
	SHOPMODULE_MODE_TALK = 1 -- Old system used before Tibia 8.2: sell/buy item name
	SHOPMODULE_MODE_TRADE = 2 -- Trade window system introduced in Tibia 8.2
	SHOPMODULE_MODE_BOTH = 3 -- Both working at one time

	-- Used shop mode
	SHOPMODULE_MODE = SHOPMODULE_MODE_BOTH

	Modules = {
		parseableModules = {}
	}

	StdModule = {}

	-- These callback function must be called with parameters.npcHandler = npcHandler in the parameters table or they will not work correctly.
	-- Notice: The members of StdModule have not yet been tested. If you find any bugs, please report them to me.
	-- Usage:
		-- keywordHandler:addKeyword({'offer'}, StdModule.say, {npcHandler = npcHandler, text = 'I sell many powerful melee weapons.'})
	function StdModule.say(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			error('StdModule.say called without any npcHandler instance.')
		end

		local onlyFocus = (parameters.onlyFocus == nil or parameters.onlyFocus == true)
		if(not npcHandler:isFocused(cid) and onlyFocus) then
			return false
		end

		local parseInfo = {[TAG_PLAYERNAME] = getCreatureName(cid)}
		npcHandler:say(npcHandler:parseMessage(parameters.text or parameters.message, parseInfo), cid, parameters.publicize and true)
		if(parameters.reset == true) then
			npcHandler:resetNpc()
		elseif(parameters.moveup ~= nil and type(parameters.moveup) == 'number') then
			npcHandler.keywordHandler:moveUp(parameters.moveup)
		end

		return true
	end

	--Usage:
		-- local node1 = keywordHandler:addKeyword({'promot'}, StdModule.say, {npcHandler = npcHandler, text = 'I can promote you for 20000 gold coins. Do you want me to promote you?'})
		-- 		node1:addChildKeyword({'yes'}, StdModule.promotePlayer, {npcHandler = npcHandler, cost = 20000, promotion = 1, level = 20}, text = 'Congratulations! You are now promoted.')
		-- 		node1:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, text = 'Alright then, come back when you are ready.'}, reset = true)
	function StdModule.promotePlayer(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			error('StdModule.promotePlayer called without any npcHandler instance.')
		end

		if(not npcHandler:isFocused(cid)) then
			return false
		end

		if(isPlayerPremiumCallback(cid) or not getBooleanFromString(getConfigValue('premiumForPromotion')) or not(parameters.premium)) then
			if(getPlayerPromotionLevel(cid) >= parameters.promotion) then
				npcHandler:say('You are already promoted!', cid)
			elseif(getPlayerLevel(cid) < parameters.level) then
				npcHandler:say('I am sorry, but I can only promote you once you have reached level ' .. parameters.level .. '.', cid)
			elseif(not doPlayerRemoveMoney(cid, parameters.cost)) then
				npcHandler:say('You do not have enough money!', cid)
			else
				setPlayerPromotionLevel(cid, parameters.promotion)
				npcHandler:say(parameters.text, cid)
			end
		else
			npcHandler:say("You need a premium account in order to get promoted.", cid)
		end

		npcHandler:resetNpc()
		return true
	end

	function StdModule.learnSpell(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			error('StdModule.learnSpell called without any npcHandler instance.')
		end

		if(not npcHandler:isFocused(cid)) then
			return false
		end

		if(isPlayerPremiumCallback(cid) or not(parameters.premium)) then
			if(getPlayerLearnedInstantSpell(cid, parameters.spellName)) then
				npcHandler:say('You already know this spell.', cid)
			elseif(getPlayerLevel(cid) < parameters.level) then
				npcHandler:say('You need to obtain a level of ' .. parameters.level .. ' or higher to be able to learn ' .. parameters.spellName .. '.', cid)
			elseif(getPlayerVocation(cid) ~= parameters.vocation and getPlayerVocation(cid) ~= parameters.vocation + 4 and vocation ~= 9) then
				npcHandler:say('This spell is not for your vocation', cid)
			elseif(not doPlayerRemoveMoney(cid, parameters.price)) then
				npcHandler:say('You do not have enough money, this spell costs ' .. parameters.price .. ' gold.', cid)
			else
				npcHandler:say('You have learned ' .. parameters.spellName .. '.', cid)
				playerLearnInstantSpell(cid, parameters.spellName)
			end
		else
			npcHandler:say('You need a premium account in order to buy ' .. parameters.spellName .. '.', cid)
		end

		npcHandler:resetNpc()
		return true
	end

	function StdModule.bless(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			error('StdModule.bless called without any npcHandler instance.')
		end

		if(not npcHandler:isFocused(cid) or getWorldType() == WORLD_TYPE_PVP_ENFORCED) then
			return false
		end

		if(isPlayerPremiumCallback(cid) or not getBooleanFromString(getConfigValue('blessingsOnlyPremium')) or not parameters.premium) then
			local price = parameters.baseCost
			if(getPlayerLevel(cid) > parameters.startLevel) then
				price = (price + ((math.min(parameters.endLevel, getPlayerLevel(cid)) - parameters.startLevel) * parameters.levelCost))
			end

			if(getPlayerBlessing(cid, parameters.number)) then
				npcHandler:say("Gods have already blessed you with this blessing!", cid)
			elseif(not doPlayerRemoveMoney(cid, price)) then
				npcHandler:say("You don't have enough money for blessing.", cid)
			else
				npcHandler:say("You have been blessed by one of the five gods!", cid)
				doPlayerAddBlessing(cid, parameters.number)
			end
		else
			npcHandler:say('You need a premium account in order to be blessed.', cid)
		end

		npcHandler:resetNpc()
		return true
	end

	function StdModule.travel(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			error('StdModule.travel called without any npcHandler instance.')
		end

		if(not npcHandler:isFocused(cid)) then
			return false
		end

		if(parameters.premium and not isPlayerPremiumCallback(cid)) then
			npcHandler:say('I can only allow premium players to travel with me.', cid)
		elseif(parameters.level ~= nil and getPlayerLevel(cid) < parameters.level) then
			npcHandler:say('You must reach level ' .. parameters.level .. ' before I can let you go there.', cid)
		elseif(parameters.storage ~= nil and getPlayerStorageValue(cid, parameters.storage) <= 0) then
			npcHandler:say(parameters.storageInfo or 'You may not travel here.', cid)
		elseif(not doPlayerRemoveMoney(cid, parameters.cost)) then
			npcHandler:say('You do not have enough money!', cid)
		elseif(isPlayerPzLocked(cid)) then
			npcHandler:say('Get out of there with this blood.', cid)
		else
			doTeleportThing(cid, parameters.destination, 0)
			doSendMagicEffect(parameters.destination, 10)
		end

		npcHandler:resetNpc()
		return true
	end

	FocusModule = {
		npcHandler = nil
	}

	-- Creates a new instance of FocusModule without an associated NpcHandler.
	function FocusModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	-- Inits the module and associates handler to it.
	function FocusModule:init(handler)
		self.npcHandler = handler
		for i, word in pairs(FOCUS_GREETWORDS) do
			local obj = {}
			table.insert(obj, word)
			obj.callback = FOCUS_GREETWORDS.callback or FocusModule.messageMatcher
			handler.keywordHandler:addKeyword(obj, FocusModule.onGreet, {module = self})
		end

		for i, word in pairs(FOCUS_FAREWELLWORDS) do
			local obj = {}
			table.insert(obj, word)
			obj.callback = FOCUS_FAREWELLWORDS.callback or FocusModule.messageMatcher
			handler.keywordHandler:addKeyword(obj, FocusModule.onFarewell, {module = self})
		end

		return true
	end

	-- Greeting callback function.
	function FocusModule.onGreet(cid, message, keywords, parameters)
		parameters.module.npcHandler:onGreet(cid)
		return true
	end

	-- UnGreeting callback function.
	function FocusModule.onFarewell(cid, message, keywords, parameters)
		if(parameters.module.npcHandler:isFocused(cid)) then
			parameters.module.npcHandler:onFarewell(cid)
			return true
		else
			return false
		end
	end

	-- Custom message matching callback function for greeting messages.
	function FocusModule.messageMatcher(keywords, message)
		local spectators = getSpectators(getCreaturePosition(getNpcId()), 7, 7)
		for i, word in pairs(keywords) do
			if(type(word) == 'string') then
				if(string.find(message, word) and not string.find(message, '[%w+]' .. word) and not string.find(message, word .. '[%w+]')) then
					if(string.find(message, getCreatureName(getNpcId()))) then
						return true
					end

					for i, uid in ipairs(spectators) do
						if(string.find(message, getCreatureName(uid))) then
							return false
						end
					end

					return true
				end
			end
		end

		return false
	end

	KeywordModule = {
		npcHandler = nil
	}
	-- Add it to the parseable module list.
	Modules.parseableModules['module_keywords'] = KeywordModule

	function KeywordModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	function KeywordModule:init(handler)
		self.npcHandler = handler
		return true
	end

	-- Parses all known parameters.
	function KeywordModule:parseParameters()
		local ret = NpcSystem.getParameter('keywords')
		if(ret ~= nil) then
			self:parseKeywords(ret)
		end
	end

	function KeywordModule:parseKeywords(data)
		local n = 1
		for keys in string.gmatch(data, '[^;]+') do
			local i = 1

			local keywords = {}
			for temp in string.gmatch(keys, '[^,]+') do
				table.insert(keywords, temp)
				i = i + 1
			end

			if(i ~= 1) then
				local reply = NpcSystem.getParameter('keyword_reply' .. n)
				if(reply ~= nil) then
					self:addKeyword(keywords, reply)
				else
					print('[Warning] NpcSystem:', 'Parameter \'' .. 'keyword_reply' .. n .. '\' missing. Skipping...')
				end
			else
				print('[Warning] NpcSystem:', 'No keywords found for keyword set #' .. n .. '. Skipping...')
			end

			n = n+1
		end
	end

	function KeywordModule:addKeyword(keywords, reply)
		self.npcHandler.keywordHandler:addKeyword(keywords, StdModule.say, {npcHandler = self.npcHandler, onlyFocus = true, text = reply, reset = true})
	end

	TravelModule = {
		npcHandler = nil,
		destinations = nil,
		yesNode = nil,
		noNode = nil,
	}
	-- Add it to the parseable module list.
	Modules.parseableModules['module_travel'] = TravelModule

	function TravelModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	function TravelModule:init(handler)
		self.npcHandler = handler
		self.yesNode = KeywordNode:new(SHOP_YESWORD, TravelModule.onConfirm, {module = self})
		self.noNode = KeywordNode:new(SHOP_NOWORD, TravelModule.onDecline, {module = self})
		self.destinations = {}
		return true
	end

	-- Parses all known parameters.
	function TravelModule:parseParameters()
		local ret = NpcSystem.getParameter('travel_destinations')
		if(ret ~= nil) then
			self:parseDestinations(ret)

			self.npcHandler.keywordHandler:addKeyword({'destination'}, TravelModule.listDestinations, {module = self})
			self.npcHandler.keywordHandler:addKeyword({'where'}, TravelModule.listDestinations, {module = self})
			self.npcHandler.keywordHandler:addKeyword({'travel'}, TravelModule.listDestinations, {module = self})

		end
	end

	function TravelModule:parseDestinations(data)
		for destination in string.gmatch(data, '[^;]+') do
			local i = 1

			local name = nil
			local x = nil
			local y = nil
			local z = nil
			local cost = nil
			local premium = false

			for temp in string.gmatch(destination, '[^,]+') do
				if(i == 1) then
					name = temp
				elseif(i == 2) then
					x = tonumber(temp)
				elseif(i == 3) then
					y = tonumber(temp)
				elseif(i == 4) then
					z = tonumber(temp)
				elseif(i == 5) then
					cost = tonumber(temp)
				elseif(i == 6) then
					premium = temp == 'true'
				else
					print('[Warning] NpcSystem:', 'Unknown parameter found in travel destination parameter.', temp, destination)
				end
				i = i + 1
			end

			if(name ~= nil and x ~= nil and y ~= nil and z ~= nil and cost ~= nil) then
				self:addDestination(name, {x=x, y=y, z=z}, cost, premium)
			else
				print('[Warning] NpcSystem:', 'Parameter(s) missing for travel destination:', name, x, y, z, cost, premium)
			end
		end
	end

	function TravelModule:addDestination(name, position, price, premium)
		table.insert(self.destinations, name)

		local parameters = {
			cost = price,
			destination = position,
			premium = premium,
			module = self
		}
		local keywords = {}
		table.insert(keywords, name)

		local keywords2 = {}
		table.insert(keywords2, 'bring me to ' .. name)
		local node = self.npcHandler.keywordHandler:addKeyword(keywords, TravelModule.travel, parameters)
		self.npcHandler.keywordHandler:addKeyword(keywords2, TravelModule.bringMeTo, parameters)
		node:addChildKeywordNode(self.yesNode)
		node:addChildKeywordNode(self.noNode)
	end

	function TravelModule.travel(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local npcHandler = module.npcHandler

		local cost = parameters.cost
		local destination = parameters.destination
		local premium = parameters.premium

		module.npcHandler:say('Do you want to travel to ' .. keywords[1] .. ' for ' .. cost .. ' gold coins?', cid)
		return true

	end

	function TravelModule.onConfirm(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local npcHandler = module.npcHandler

		local parentParameters = node:getParent():getParameters()
		local cost = parentParameters.cost
		local destination = parentParameters.destination
		local premium = parentParameters.premium

		if(isPlayerPremiumCallback(cid) or parameters.premium ~= true) then
			if(not doPlayerRemoveMoney(cid, cost)) then
				npcHandler:say('You do not have enough money!', cid)
			elseif(isPlayerPzLocked(cid)) then
				npcHandler:say('Get out of there with this blood.', cid)
			else
				npcHandler:say('It was a pleasure doing business with you.', cid)
				npcHandler:releaseFocus(cid)
				doTeleportThing(cid, destination, 0)
				doSendMagicEffect(destination, 10)
			end
		else
			npcHandler:say('I can only allow premium players to travel there.', cid)
		end

		npcHandler:resetNpc()
		return true
	end

	-- onDecline keyword callback function. Generally called when the player sais 'no' after wanting to buy an item.
	function TravelModule.onDecline(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end
		local parentParameters = node:getParent():getParameters()
		local parseInfo = {
			[TAG_PLAYERNAME] = getCreatureName(cid),
		}
		local msg = module.npcHandler:parseMessage(module.npcHandler:getMessage(MESSAGE_DECLINE), parseInfo)
		module.npcHandler:say(msg, cid)
		module.npcHandler:resetNpc()
		return true
	end

	function TravelModule.bringMeTo(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local cost = parameters.cost
		local destination = parameters.destination
		local premium = parameters.premium

		if(isPlayerPremiumCallback(cid) or parameters.premium ~= true) then
			if(doPlayerRemoveMoney(cid, cost)) then
				doTeleportThing(cid, destination, 0)
				doSendMagicEffect(destination, 10)
			end
		end
		return true
	end

	function TravelModule.listDestinations(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local msg = 'I can bring you to '
		--local i = 1
		local maxn = table.maxn(module.destinations)
		for i, destination in pairs(module.destinations) do
			msg = msg .. "{" .. destination .. "}"
			if(i == maxn -1) then
				msg = msg .. ' and '
			elseif(i == maxn) then
				msg = msg .. '.'
			else
				msg = msg .. ', '
			end
			i = i + 1
		end

		module.npcHandler:say(msg, cid)
		module.npcHandler:resetNpc()
		return true
	end

	ShopModule = {
		npcHandler = nil,
		yesNode = nil,
		noNode = nil,
		noText = '',
		maxCount = 100,
		amount = 0
	}

	-- Add it to the parseable module list.
	Modules.parseableModules['module_shop'] = ShopModule

	-- Creates a new instance of ShopModule
	function ShopModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	-- Parses all known parameters.
	function ShopModule:parseParameters()
		local ret = NpcSystem.getParameter('shop_buyable')
		if(ret ~= nil) then
			self:parseBuyable(ret)
		end

		local ret = NpcSystem.getParameter('shop_sellable')
		if(ret ~= nil) then
			self:parseSellable(ret)
		end

		local ret = NpcSystem.getParameter('shop_buyable_containers')
		if(ret ~= nil) then
			self:parseBuyableContainers(ret)
		end
	end

	-- Parse a string contaning a set of buyable items.
	function ShopModule:parseBuyable(data)
		for item in string.gmatch(data, '[^;]+') do
			local i = 1

			local name = nil
			local itemid = nil
			local cost = nil
			local subType = nil
			local realName = nil

			for temp in string.gmatch(item, '[^,]+') do
				if(i == 1) then
					name = temp
				elseif(i == 2) then
					itemid = tonumber(temp)
				elseif(i == 3) then
					cost = tonumber(temp)
				elseif(i == 4) then
					subType = tonumber(temp)
				elseif(i == 5) then
					realName = temp
				else
					print('[Warning] NpcSystem:', 'Unknown parameter found in buyable items parameter.', temp, item)
				end
				i = i + 1
			end

			if(SHOPMODULE_MODE == SHOPMODULE_MODE_TRADE) then
				if(itemid ~= nil and cost ~= nil) then
					if((isItemRune(itemid) or isItemFluidContainer(itemid)) and subType == nil) then
						print('[Warning] NpcSystem:', 'SubType missing for parameter item:', item)
					else
						self:addBuyableItem(nil, itemid, cost, subType, realName)
					end
				else
					print('[Warning] NpcSystem:', 'Parameter(s) missing for item:', itemid, cost)
				end
			else
				if(name ~= nil and itemid ~= nil and cost ~= nil) then
					if((isItemRune(itemid) or isItemFluidContainer(itemid)) and subType == nil) then
						print('[Warning] NpcSystem:', 'SubType missing for parameter item:', item)
					else
						local names = {}
						table.insert(names, name)
						self:addBuyableItem(names, itemid, cost, subType, realName)
					end
				else
					print('[Warning] NpcSystem:', 'Parameter(s) missing for item:', name, itemid, cost)
				end
			end
		end
	end

	-- Parse a string contaning a set of sellable items.
	function ShopModule:parseSellable(data)
		for item in string.gmatch(data, '[^;]+') do
			local i = 1

			local name = nil
			local itemid = nil
			local cost = nil
			local realName = nil

			for temp in string.gmatch(item, '[^,]+') do
				if(i == 1) then
					name = temp
				elseif(i == 2) then
					itemid = tonumber(temp)
				elseif(i == 3) then
					cost = tonumber(temp)
				elseif(i == 4) then
					realName = temp
				else
					print('[Warning] NpcSystem:', 'Unknown parameter found in sellable items parameter.', temp, item)
				end
				i = i + 1
			end

			if(SHOPMODULE_MODE == SHOPMODULE_MODE_TRADE) then
				if(itemid ~= nil and cost ~= nil) then
					self:addSellableItem(nil, itemid, cost, realName)
				else
					print('[Warning] NpcSystem:', 'Parameter(s) missing for item:', itemid, cost)
				end
			else
				if(name ~= nil and itemid ~= nil and cost ~= nil) then
					local names = {}
					table.insert(names, name)
					self:addSellableItem(names, itemid, cost, realName)
				else
					print('[Warning] NpcSystem:', 'Parameter(s) missing for item:', name, itemid, cost)
				end
			end
		end
	end

	-- Parse a string contaning a set of buyable items.
	function ShopModule:parseBuyableContainers(data)
		for item in string.gmatch(data, '[^;]+') do
			local i = 1

			local name = nil
			local container = nil
			local itemid = nil
			local cost = nil
			local subType = nil
			local realName = nil

			for temp in string.gmatch(item, '[^,]+') do
				if(i == 1) then
					name = temp
				elseif(i == 2) then
					itemid = tonumber(temp)
				elseif(i == 3) then
					itemid = tonumber(temp)
				elseif(i == 4) then
					cost = tonumber(temp)
				elseif(i == 5) then
					subType = tonumber(temp)
				elseif(i == 6) then
					realName = temp
				else
					print('[Warning] NpcSystem:', 'Unknown parameter found in buyable items parameter.', temp, item)
				end
				i = i + 1
			end

			if(name ~= nil and container ~= nil and itemid ~= nil and cost ~= nil) then
				if((isItemRune(itemid) or isItemFluidContainer(itemid)) and subType == nil) then
					print('[Warning] NpcSystem:', 'SubType missing for parameter item:', item)
				else
					local names = {}
					table.insert(names, name)
					self:addBuyableItemContainer(names, container, itemid, cost, subType, realName)
				end
			else
				print('[Warning] NpcSystem:', 'Parameter(s) missing for item:', name, container, itemid, cost)
			end
		end
	end

	-- Initializes the module and associates handler to it.
	function ShopModule:init(handler)
		self.npcHandler = handler
		self.yesNode = KeywordNode:new(SHOP_YESWORD, ShopModule.onConfirm, {module = self})
		self.noNode = KeywordNode:new(SHOP_NOWORD, ShopModule.onDecline, {module = self})
		self.noText = handler:getMessage(MESSAGE_DECLINE)

		if(SHOPMODULE_MODE ~= SHOPMODULE_MODE_TALK) then
			for i, word in pairs(SHOP_TRADEREQUEST) do
				local obj = {}
				table.insert(obj, word)
				obj.callback = SHOP_TRADEREQUEST.callback or ShopModule.messageMatcher
				handler.keywordHandler:addKeyword(obj, ShopModule.requestTrade, {module = self})
			end
		end

		return true
	end

	-- Custom message matching callback function for requesting trade messages.
	function ShopModule.messageMatcher(keywords, message)
		for i, word in pairs(keywords) do
			if(type(word) == 'string') then
				if string.find(message, word) and not string.find(message, '[%w+]' .. word) and not string.find(message, word .. '[%w+]') then
					return true
				end
			end
		end

		return false
	end

	-- Resets the module-specific variables.
	function ShopModule:reset()
		self.amount = 0
	end

	-- Function used to match a number value from a string.
	function ShopModule:getCount(message)
		local ret = 1
		local b, e = string.find(message, PATTERN_COUNT)
		if b ~= nil and e ~= nil then
			ret = tonumber(string.sub(message, b, e))
		end

		if(ret <= 0) then
			ret = 1
		elseif(ret > self.maxCount) then
			ret = self.maxCount
		end

		return ret
	end

	-- Adds a new buyable item.
	--	names = A table containing one or more strings of alternative names to this item. Used only for old buy/sell system.
	--	itemid = The itemid of the buyable item
	--	cost = The price of one single item
	--	subType - The subType of each rune or fluidcontainer item. Can be left out if it is not a rune/fluidcontainer. Default value is 1.
	--	realName - The real, full name for the item. Will be used as ITEMNAME in MESSAGE_ONBUY and MESSAGE_ONSELL if defined. Default value is nil (getItemNameById will be used)
	function ShopModule:addBuyableItem(names, itemid, cost, subType, realName)
		if(SHOPMODULE_MODE ~= SHOPMODULE_MODE_TALK) then
			if(self.npcHandler.shopItems[itemid] == nil) then
				self.npcHandler.shopItems[itemid] = {buyPrice = -1, sellPrice = -1, subType = 1, realName = ""}
			end

			self.npcHandler.shopItems[itemid].buyPrice = cost
			self.npcHandler.shopItems[itemid].realName = realName or getItemNameById(itemid)
			self.npcHandler.shopItems[itemid].subType = subType or 1
		end

		if(names ~= nil and SHOPMODULE_MODE ~= SHOPMODULE_MODE_TRADE) then
			for i, name in pairs(names) do
				local parameters = {
						itemid = itemid,
						cost = cost,
						eventType = SHOPMODULE_BUY_ITEM,
						module = self,
						realName = realName or getItemNameById(itemid),
						subType = subType or 1
					}

				keywords = {}
				table.insert(keywords, 'buy')
				table.insert(keywords, name)
				local node = self.npcHandler.keywordHandler:addKeyword(keywords, ShopModule.tradeItem, parameters)
				node:addChildKeywordNode(self.yesNode)
				node:addChildKeywordNode(self.noNode)
			end
		end
	end

	-- Adds a new buyable container of items.
	--	names = A table containing one or more strings of alternative names to this item.
	--	container = Backpack, bag or any other itemid of container where bought items will be stored
	--	itemid = The itemid of the buyable item
	--	cost = The price of one single item
	--	subType - The subType of each rune or fluidcontainer item. Can be left out if it is not a rune/fluidcontainer. Default value is 1.
	--	realName - The real, full name for the item. Will be used as ITEMNAME in MESSAGE_ONBUY and MESSAGE_ONSELL if defined. Default value is nil (getItemNameById will be used)
	function ShopModule:addBuyableItemContainer(names, container, itemid, cost, subType, realName)
		if(names ~= nil) then
			for i, name in pairs(names) do
				local parameters = {
						container = container,
						itemid = itemid,
						cost = cost,
						eventType = SHOPMODULE_BUY_ITEM_CONTAINER,
						module = self,
						realName = realName or getItemNameById(itemid),
						subType = subType or 1
					}

				keywords = {}
				table.insert(keywords, 'buy')
				table.insert(keywords, name)
				local node = self.npcHandler.keywordHandler:addKeyword(keywords, ShopModule.tradeItem, parameters)
				node:addChildKeywordNode(self.yesNode)
				node:addChildKeywordNode(self.noNode)
			end
		end
	end

	-- Adds a new sellable item.
	--	names = A table containing one or more strings of alternative names to this item. Used only by old buy/sell system.
	--	itemid = The itemid of the sellable item
	--	cost = The price of one single item
	--	realName - The real, full name for the item. Will be used as ITEMNAME in MESSAGE_ONBUY and MESSAGE_ONSELL if defined. Default value is nil (getItemNameById will be used)
	function ShopModule:addSellableItem(names, itemid, cost, realName)
		if(SHOPMODULE_MODE ~= SHOPMODULE_MODE_TALK) then
			if(self.npcHandler.shopItems[itemid] == nil) then
				self.npcHandler.shopItems[itemid] = {buyPrice = -1, sellPrice = -1, subType = 1, realName = ""}
			end

			self.npcHandler.shopItems[itemid].sellPrice = cost
			self.npcHandler.shopItems[itemid].realName = realName or getItemNameById(itemid)
		end

		if(names ~= nil and SHOPMODULE_MODE ~= SHOPMODULE_MODE_TRADE) then
			for i, name in pairs(names) do
				local parameters = {
						itemid = itemid,
						cost = cost,
						eventType = SHOPMODULE_SELL_ITEM,
						module = self,
						realName = realName or getItemNameById(itemid)
					}

				keywords = {}
				table.insert(keywords, 'sell')
				table.insert(keywords, name)
				local node = self.npcHandler.keywordHandler:addKeyword(keywords, ShopModule.tradeItem, parameters)
				node:addChildKeywordNode(self.yesNode)
				node:addChildKeywordNode(self.noNode)
			end
		end
	end

	-- onModuleReset callback function. Calls ShopModule:reset()
	function ShopModule:callbackOnModuleReset()
		self:reset()
		return true
	end

	-- Callback onBuy() function. If you wish, you can change certain Npc to use your onBuy().
	function ShopModule:callbackOnBuy(cid, itemid, subType, amount, ignoreCap, inBackpacks)
		if(self.npcHandler.shopItems[itemid] == nil) then
			error("[ShopModule.onBuy]", "items[itemid] == nil")
			return false
		end

		if(self.npcHandler.shopItems[itemid].buyPrice == -1) then
			error("[ShopModule.onSell]", "Attempt to buy a non-buyable item")
			return false
		end

		local backpack = 1988
		local totalCost = amount * self.npcHandler.shopItems[itemid].buyPrice
		if(inBackpacks) then
			totalCost = totalCost + (math.max(1, math.floor(amount / getContainerCapById(backpack))) * 20)
		end

		local parseInfo = {
			[TAG_PLAYERNAME] = getPlayerName(cid),
			[TAG_ITEMCOUNT] = amount,
			[TAG_TOTALCOST] = totalCost,
			[TAG_ITEMNAME] = self.npcHandler.shopItems[itemid].realName
		}

		if(getPlayerMoney(cid) < totalCost) then
			local msg = self.npcHandler:getMessage(MESSAGE_NEEDMONEY)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			doPlayerSendCancel(cid, msg)
			return false
		end

		local subType = self.npcHandler.shopItems[itemid].subType or 1
		local a, b = doNpcSellItem(cid, itemid, amount, subType, ignoreCap, inBackpacks, backpack)
		if(a < amount) then
			local msgId = MESSAGE_NEEDMORESPACE
			if(a == 0) then
				msgId = MESSAGE_NEEDSPACE
			end

			local msg = self.npcHandler:getMessage(msgId)
			parseInfo[TAG_ITEMCOUNT] = a
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			doPlayerSendCancel(cid, msg)
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end

			if(a > 0) then
				doPlayerRemoveMoney(cid, ((a * self.npcHandler.shopItems[itemid].buyPrice) + (b * 20)))
				return true
			end

			return false
		else
			local msg = self.npcHandler:getMessage(MESSAGE_BOUGHT)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, msg)
			doPlayerRemoveMoney(cid, totalCost)
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end

			return true
		end
	end

	-- Callback onSell() function. If you wish, you can change certain Npc to use your onSell().
	function ShopModule:callbackOnSell(cid, itemid, subType, amount, ignoreCap, inBackpacks)
		if(self.npcHandler.shopItems[itemid] == nil) then
			error("[ShopModule.onSell]", "items[itemid] == nil")
			return false
		end

		if(self.npcHandler.shopItems[itemid].sellPrice == -1) then
			error("[ShopModule.onSell]", "Attempt to sell a non-sellable item")
			return false
		end

		local parseInfo = {
			[TAG_PLAYERNAME] = getPlayerName(cid),
			[TAG_ITEMCOUNT] = amount,
			[TAG_TOTALCOST] = amount * self.npcHandler.shopItems[itemid].sellPrice,
			[TAG_ITEMNAME] = self.npcHandler.shopItems[itemid].realName
		}

		if(subType < 1) then
			subType = -1
		end

		if(doPlayerRemoveItem(cid, itemid, amount, subType)) then
			local msg = self.npcHandler:getMessage(MESSAGE_SOLD)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, msg)
			doPlayerAddMoney(cid, amount * self.npcHandler.shopItems[itemid].sellPrice)
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end
			return true
		else
			local msg = self.npcHandler:getMessage(MESSAGE_NEEDITEM)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			doPlayerSendCancel(cid, msg)
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end
			return false
		end
	end

	-- Callback for requesting a trade window with the NPC.
	function ShopModule.requestTrade(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local itemWindow = {}
		for itemid, attr in pairs(module.npcHandler.shopItems) do
			local item = {id = itemid, buy = attr.buyPrice, sell = attr.sellPrice, subType = attr.subType, name = attr.realName}
			table.insert(itemWindow, item)
		end

		if(itemWindow[1] == nil) then
			local parseInfo = { [TAG_PLAYERNAME] = getPlayerName(cid) }
			local msg = module.npcHandler:parseMessage(module.npcHandler:getMessage(MESSAGE_NOSHOP), parseInfo)
			module.npcHandler:say(msg, cid)
			return true
		end

		local parseInfo = { [TAG_PLAYERNAME] = getPlayerName(cid) }
		local msg = module.npcHandler:parseMessage(module.npcHandler:getMessage(MESSAGE_SENDTRADE), parseInfo)
		openShopWindow(cid, itemWindow,
			function(cid, itemid, subType, amount, ignoreCap, inBackpacks) module.npcHandler:onBuy(cid, itemid, subType, amount, ignoreCap, inBackpacks) end,
			function(cid, itemid, subType, amount, ignoreCap, inBackpacks) module.npcHandler:onSell(cid, itemid, subType, amount, ignoreCap, inBackpacks) end)
		module.npcHandler:say(msg, cid)
		return true
	end

	-- onConfirm keyword callback function. Sells/buys the actual item.
	function ShopModule.onConfirm(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local parentParameters = node:getParent():getParameters()
		local parseInfo = {
			[TAG_PLAYERNAME] = getPlayerName(cid),
			[TAG_ITEMCOUNT] = module.amount,
			[TAG_TOTALCOST] = parentParameters.cost * module.amount,
			[TAG_ITEMNAME] = parentParameters.realName
		}

		if(parentParameters.eventType == SHOPMODULE_SELL_ITEM) then
			local ret = doPlayerSellItem(cid, parentParameters.itemid, module.amount, parentParameters.cost * module.amount)
			if(ret) then
				local msg = module.npcHandler:getMessage(MESSAGE_ONSELL)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg, cid)
			else
				local msg = module.npcHandler:getMessage(MESSAGE_MISSINGITEM)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg, cid)
			end
		elseif(parentParameters.eventType == SHOPMODULE_BUY_ITEM) then
			local ret = doPlayerBuyItem(cid, parentParameters.itemid, module.amount, parentParameters.cost * module.amount, parentParameters.subType)
			if(ret) then
				if parentParameters.itemid == ITEM_PARCEL then
					doPlayerBuyItem(cid, ITEM_LABEL, module.amount, 0, parentParameters.subType)
				end
				local msg = module.npcHandler:getMessage(MESSAGE_ONBUY)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg, cid)
			else
				local msg = module.npcHandler:getMessage(MESSAGE_MISSINGMONEY)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg, cid)
			end
		elseif(parentParameters.eventType == SHOPMODULE_BUY_ITEM_CONTAINER) then
			local ret = doPlayerBuyItemContainer(cid, parentParameters.container, parentParameters.itemid, module.amount, parentParameters.cost * module.amount, parentParameters.subType)
			if(ret) then
				local msg = module.npcHandler:getMessage(MESSAGE_ONBUY)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg, cid)
			else
				local msg = module.npcHandler:getMessage(MESSAGE_MISSINGMONEY)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg, cid)
			end
		end

		module.npcHandler:resetNpc()
		return true
	end

	-- onDecliune keyword callback function. Generally called when the player sais 'no' after wanting to buy an item.
	function ShopModule.onDecline(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local parentParameters = node:getParent():getParameters()
		local parseInfo = {
			[TAG_PLAYERNAME] = getPlayerName(cid),
			[TAG_ITEMCOUNT] = module.amount,
			[TAG_TOTALCOST] = parentParameters.cost * module.amount,
			[TAG_ITEMNAME] = parentParameters.realName
		}

		local msg = module.npcHandler:parseMessage(module.noText, parseInfo)
		module.npcHandler:say(msg, cid)
		module.npcHandler:resetNpc()
		return true
	end

	-- tradeItem callback function. Makes the npc say the message defined by MESSAGE_BUY or MESSAGE_SELL
	function ShopModule.tradeItem(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local count = module:getCount(message)
		module.amount = count
		local parseInfo = {
			[TAG_PLAYERNAME] = getPlayerName(cid),
			[TAG_ITEMCOUNT] = module.amount,
			[TAG_TOTALCOST] = parameters.cost * module.amount,
			[TAG_ITEMNAME] = parameters.realName
		}

		if(parameters.eventType == SHOPMODULE_SELL_ITEM) then
			local msg = module.npcHandler:getMessage(MESSAGE_SELL)
			msg = module.npcHandler:parseMessage(msg, parseInfo)
			module.npcHandler:say(msg, cid)
		elseif(parameters.eventType == SHOPMODULE_BUY_ITEM) then
			local msg = module.npcHandler:getMessage(MESSAGE_BUY)
			msg = module.npcHandler:parseMessage(msg, parseInfo)
			module.npcHandler:say(msg, cid)
		elseif(parameters.eventType == SHOPMODULE_BUY_ITEM_CONTAINER) then
			local msg = module.npcHandler:getMessage(MESSAGE_BUY)
			msg = module.npcHandler:parseMessage(msg, parseInfo)
			module.npcHandler:say(msg, cid)
		end

		return true
	end
end
