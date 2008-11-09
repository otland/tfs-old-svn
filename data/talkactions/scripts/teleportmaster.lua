function onSay(cid, words, param)
	if(param ~= "") then
		cid = getPlayerByNameWildcard(param)
	end

	local tmp = getCreaturePosition(cid)
	local pos = getClosestFreeTile(cid, getTownTemplePosition(getPlayerTown(cid)))
	if(doTeleportThing(cid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(cid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
