table.append = table.insert
table.empty = function (t)
	return next(t) == nil
end

table.size = function (t)
	return #t
end

table.find = function (table, value, sensitive)
	local sensitive = sensitive or true
	if(not sensitive and type(value) == "string") then
		for i, v in pairs(table) do
			if(type(v) == "string") then
				if(v:lower() == value:lower()) then
					return i
				end
			end
		end

		return nil
	end

	for i, v in pairs(table) do
		if(v == value) then
			return i
		end
	end

	return nil
end

table.contains = function (txt, str)
	for i, v in pairs(str) do
		if(txt:find(v) and not txt:find('(%w+)' .. v) and not txt:find(v .. '(%w+)')) then
			return true
		end
	end

	return false
end
table.isStrIn = table.contains

table.count = function (table, item)
	local count = 0
	for i, n in pairs(table) do
		if(item == n) then
			count = count + 1
		end
	end

	return count
end
table.countElements = table.count

table.getCombinations = function (table, num)
	local a, number, select, newlist = {}, #table, num, {}
	for i = 1, select do
		a[#a + 1] = i
	end

	local newthing = {}
	while(true) do
		local newrow = {}
		for i = 1, select do
			newrow[#newrow + 1] = table[a[i]]
		end

		newlist[#newlist + 1] = newrow
		i = select
		while(a[i] == (number - select + i)) do
			i = i - 1
		end

		if(i < 1) then
			break
		end

		a[i] = a[i] + 1
		for j = i, select do
			a[j] = a[i] + j - i
		end
	end

	return newlist
end

function table.serialize(x, recur)
	local t = type(x)
	recur = recur or {}

	if(t == nil) then
		return "nil"
	elseif(t == "string") then
		return string.format("%q", x)
	elseif(t == "number") then
		return tostring(x)
	elseif(t == "boolean") then
		return t and "true" or "false"
	elseif(getmetatable(x)) then
		error("Can not serialize a table that has a metatable associated with it.")
	elseif(t == "table") then
		if(table.find(recur, x)) then
			error("Can not serialize recursive tables.")
		end
		table.append(recur, x)

		local s = "{"
		for k, v in pairs(x) do
			s = s .. "[" .. table.serialize(k, recur) .. "]" .. " = " .. table.serialize(v, recur) .. ", "
		end

		return s:sub(0, s:len() - 2) .. "}"
	end

	error("Can not serialize value of type '" .. t .. "'.")
end

function table.unserialize(str)
	return loadstring("return " .. str)()
end
