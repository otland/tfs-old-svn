function isSorcerer(cid)
	if(not isPlayer(cid)) then
		debugPrint("isSorcerer: Player not found.")
		return false
	end

	return isInArray({1, 5}, getPlayerVocation(cid))
end

function isDruid(cid)
	if(not isPlayer(cid)) then
		debugPrint("isDruid: Player not found.")
		return false
	end

	return isInArray({2, 6}, getPlayerVocation(cid))
end

function isPaladin(cid)
	if(not isPlayer(cid)) then
		debugPrint("isPaladin: Player not found.")
		return false
	end

	return isInArray({3, 7}, getPlayerVocation(cid))
end

function isKnight(cid)
	if(not isPlayer(cid)) then
		debugPrint("isKnight: Player not found.")
		return false
	end

	return isInArray({4, 8}, getPlayerVocation(cid))
end

function isRookie(cid)
	if(not isPlayer(cid)) then
		debugPrint("isRookie: Player not found.")
		return false
	end

	return isInArray({0}, getPlayerVocation(cid))
end
