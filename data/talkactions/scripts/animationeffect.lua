function onSay(cid, words, param)
	param = tonumber(param)
	if(param == nil or param < 0 or param > CONST_ANI_LAST) then
		doPlayerSendCancel(cid, "Numeric param may not be lower than 0 and higher than " .. CONST_ANI_LAST .. ".")
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
