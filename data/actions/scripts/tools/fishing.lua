local config = {
	waters = {4614, 4615, 4616, 4617, 4618, 4619, 4620, 4621, 4622, 4623, 4624, 4625, 4665, 4666, 4820, 4821, 4822, 4823, 4824, 4825},
	fishable = {4608, 4609, 4610, 4611, 4612, 4613, 7236},
	spawning = {4614, 4615, 4616, 4617, 4618, 4619},
	corpses = {
		-- [corpse] = {items}
		[2025] = {
			-- {itemid, countmax, chance}
			-- TODO: Water elemental and Massive Water Elemental loot...
		}
	},
	holes = {7236},

	checkCorpseOwner = getConfigValue("checkCorpseOwner"),
	rateLoot = getConfigValue("rateLoot"),
	allowFromPz = false,
	useBait = true,
	baitCount = 1,
	fishes = 1
}

config.checkCorpseOwner = getBooleanFromString(config.checkCorpseOwner)

function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(isInArray(config.waters, itemEx.itemid)) then
		if(isInArray(config.spawnable, itemEx.itemid)) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTPOSSIBLE)
		end

		doSendMagicEffect(toPosition, CONST_ME_LOSEENERGY)
		return true
	end

	local corpse = config.corpses[itemEx.itemid]
	if(corpse ~= nil) then
		local owner = getItemAttribute(cid, "corpseowner")
		if(owner ~= 0 and owner ~= getPlayerGUID(cid) and config.checkCorpseOwner) then
			doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUARENOTTHEOWNER)
			return true
		end

		local chance, items = math.random(0, 100000) / config.rateLoot, {}
		for _, data in ipairs(corpse) then
			if(data[3] >= chance) then
				local tmp = {data[1], math.random(1, data[2])}
				table.insert(items, tmp)
			end
		end

		local itemCount = table.maxn(items)
		if(itemCount > 0) then
			local loot = items[math.random(1, itemCount)]
			doPlayerAddItem(cid, loot[1], loot[2])
		end

		doTransformItem(itemEx.uid, itemEx.uid + 1)
		return true
	end

	if(not isInArray(config.fishable, itemEx.itemid)) then
		return false
	end

	config.allowFromPz = config.allowFromPz or not getTileInfo(getThingPosition(cid)).protection
	local formula, tries = getPlayerSkill(cid, SKILL_FISHING) / 200 + 0.85 * math.random(), 1
	if(item.itemid ~= ITEM_MECHANICAL_FISHING_ROD) then
		if(config.allowFromPz and (not config.useBait or
			(getPlayerItemCount(cid, ITEM_WORM) >= config.baitCount and
			doPlayerRemoveItem(cid, ITEM_WORM, config.baitCount)))) then
			if(isInArray(config.holes, itemEx.itemid)) then
				tries = 2
				if(formula > 0.83) then
					doPlayerAddItem(cid, ITEM_RAINBOW_TROUT, config.fishes)
				elseif(formula > 0.7) then
					doPlayerAddItem(cid, ITEM_NORTHERN_PIKE, config.fishes)
				elseif(formula > 0.5) then
					doPlayerAddItem(cid, ITEM_GREEN_PERCH, config.fishes)
				else
					doPlayerAddItem(cid, ITEM_FISH, config.fishes)
				end
			elseif(formula > 0.7) then
				doPlayerAddItem(cid, ITEM_FISH, config.fishes)
				tries = 2
			end
		end
	elseif(formula > 0.7 and config.allowFromPz and (not config.useBait or
		(getPlayerItemCount(cid, ITEM_NAIL) >= config.baitCount and
		doPlayerRemoveItem(cid, ITEM_NAIL, config.baitCount)))) then
		doPlayerAddItem(cid, ITEM_MECHANICAL_FISH, config.fishes)
		tries = 2
	end

	doSendMagicEffect(toPosition, CONST_ME_LOSEENERGY)
	doPlayerAddSkillTry(cid, SKILL_FISHING, tries)
	if(tries == 1) then
		return true
	end

	if(not isInArray(config.holes, itemEx.itemid)) then
		doTransformItem(itemEx.uid, itemEx.itemid + 6)
	else
		doTransformItem(itemEx.uid, itemEx.itemid + 1)
	end

	doDecayItem(itemEx.uid)
	return true
end
