function onSay(cid, words, param)
	local n = 1
	if(param ~= "" and tonumber(param)) then
		n = tonumber(param)
	end

	local tmp = getCreaturePosition(cid)
	local pos = getCreaturePosition(cid)
	if(string.sub(words, 2, 2) == "u") then
		pos.z = pos.z - n
	else
		pos.z = pos.z + n
	end

	pos = getClosestFreeTile(cid, pos)
	if(doTeleportThing(cid, pos, TRUE) ~= LUA_ERROR and isPlayerGhost(cid) ~= TRUE) then
		doSendMagicEffect(tmp, CONST_ME_POFF)
		doSendMagicEffect(pos, CONST_ME_TELEPORT)
	end

	return TRUE
end
