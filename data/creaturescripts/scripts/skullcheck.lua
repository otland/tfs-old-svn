function onThink(cid, interval)
	local skull, skullEnd = getCreatureSkull(cid), getPlayerRedSkullEnd(cid)
	if(skullEnd > 0 and skull > SKULL_WHITE and os.time() > skullEnd and not getCreatureCondition(cid, CONDITION_INFIGHT)) then
		doPlayerSetRedSkullEnd(cid, 0, skull)
	end
end
