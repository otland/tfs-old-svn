function onUse(cid, item, fromPosition, itemEx, toPosition)
	local rand, playerPos = math.random(1, 100), getPlayerPosition(cid)

	if((rand >= 50) and (rand < 83)) then
		doSummonCreature("Spider", playerPos)
	elseif((rand >= 83) and (rand < 97)) then
		doSummonCreature("Poison Spider", playerPos)
	elseif((rand >= 97) and (rand < 100)) then
		doSummonCreature("Tarantula", playerPos)
	elseif(rand == 100) then
		doSummonCreature("Giant Spider", playerPos)
	end

	doTransformItem(item.uid,7536)

	return TRUE
end
