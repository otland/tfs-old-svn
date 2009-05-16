string.split = function (str)
	local t = {}
	local function helper(word)
		table.insert(t, word)
		return ""
	end

	if(not str:gsub("%w+", helper):find("%S")) then
		return t
	end
end

string.trim = function (str)
	return (string.gsub(str, "^%s*(.-)%s*$", "%1"))
end

string.explode = function (str, sep)
	local pos, t = 1, {}
	if #sep == 0 or #str == 0 then
		return
	end

	for s, e in function() return str:find(sep, pos) end do
		table.insert(t, str:sub(pos, s - 1):trim())
		pos = e + 1
	end

	table.insert(t, str:sub(pos):trim())
	return t
end
