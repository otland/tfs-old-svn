function onSay(cid, words, param)
	if(param == "") then
		return TRUE
	end

	local position = getCreaturePosition(cid)
	local i = 0
	while i <= 30 do
		doSendDistanceShoot(position, {x = position.x + math.random(-7, 7), y = position.y + math.random(-5, 5), z = position.z}, param)
		i = i + 1
	end
	return TRUE
end
