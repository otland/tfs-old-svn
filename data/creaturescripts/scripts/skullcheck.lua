function onThink(cid, interval)
	local redSkull = getPlayerRedSkullEnd(cid)
	if(redSkull > 0 and getCreatureSkull(cid) == SKULL_RED and os.time() > redSkull and not getCreatureCondition(cid, CONDITION_INFIGHT)) then
		doPlayerSetRedSkullEnd(cid, 0)
	end
end
