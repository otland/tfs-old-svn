function onSay(cid, words, param, channel)
	local str = ""
	for i, waypoint in ipairs(getWaypointList()) do
		str = str .. waypoint.name .. "\n"
	end

	doShowTextDialog(cid, 2160, str)
	return true
end
