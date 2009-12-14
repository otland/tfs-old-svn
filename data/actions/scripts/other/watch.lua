local config = {
	tibianTime = true,
	twentyFour = true -- only if tibianTime = false
}

function onUse(cid, item, fromPosition, itemEx, toPosition)
	local str = ""
	if(config.tibianTime) then
		local var = (os.date('%M') * 60 + os.date('%S')) / 150
		local hour = math.floor(var)

		local minute = math.floor(60 * (var - hour))
		if(hour < 10) then
			hour = '0' .. hour
		end

		if(minute < 10) then
			minute = '0' .. minute
		end

		str = hour .. ':' .. minute
	elseif(config.twentyFour) then
		str = os.date('%H:%M')
	else
		str = os.date('%I:%M %p')
	end

	doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "The time is " .. str .. ".")
	return true
end
