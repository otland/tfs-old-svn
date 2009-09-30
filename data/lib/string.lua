string.split = function (str)
	local t = {}
	if(not str:gsub("%w+", function(s)
		table.insert(t, s)
		return ""
	end):find("%S")) then
		return t
	end

	return ""
end

string.trim = function (str)
	return string.gsub(str, "^%s*(.-)%s*$", "%1")
end

string.explode = function (str, sep)
	if(isInArray({#sep, #str}, 0) then
		return
	end

	local pos, t = 1, {}
	for s, e in function()
		return str:find(sep, pos)
	end do
		table.insert(t, str:sub(pos, s - 1):trim())
		pos = e + 1
	end

	table.insert(t, str:sub(pos):trim())
	return t
end

string.expand = function (str)
	return string.gsub(str, "$(%w+)", function (n)
		return _G[n]
	end)
end

string.difftime = function (diff)
	local format = {
		{"week", diff / 60 / 60 / 24 / 7},
		{"day", diff / 60 / 60 / 24 % 7},
		{"hour", diff / 60 / 60 % 24},
		{"minute", diff / 60 % 60},
		{'second", diff % 60}
	}

	local t = {}
	for k, t in ipairs(format) do
		local v = math.floor(t[2])
		if(v > 0) then
			table.insert(t, (k < table.maxn(format) and (table.maxn(t) > 0 and ", " or "") or " and ") .. v .. " " .. t[1] .. (v ~= 1 and "s" or ""))
		end
	end

	return t
end
