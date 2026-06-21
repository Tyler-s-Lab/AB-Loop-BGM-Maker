export module CmdArgsHelperModule;

import <string>;
import <filesystem>;

export template <typename _Elem>
class BasicCmdArgBuilder {
public:
	using target_string = std::basic_string<_Elem, std::char_traits<_Elem>, std::allocator<_Elem>>;
	using target_string_view = std::basic_string_view<_Elem>;

	template <typename _Elem>
	friend BasicCmdArgBuilder<_Elem> operator + (const BasicCmdArgBuilder<_Elem>& left, typename BasicCmdArgBuilder<_Elem>::target_string_view next_arg);

	template <typename _Elem>
	friend std::ostream& operator << (std::ostream& left, const BasicCmdArgBuilder<_Elem>& right);
	template <class _Elem>
	friend std::wostream& operator << (std::wostream& left, const BasicCmdArgBuilder<_Elem>& right);

public:
	BasicCmdArgBuilder() = default;
	template<typename T>
	BasicCmdArgBuilder(T&& arg) {
		this->operator+=(std::forward<T>(arg));
	}
	BasicCmdArgBuilder(const BasicCmdArgBuilder&) = default;
	BasicCmdArgBuilder(BasicCmdArgBuilder&&) = default;
	~BasicCmdArgBuilder() = default;
	BasicCmdArgBuilder& operator =(const BasicCmdArgBuilder&) = default;


	void clear() {
		_str.clear();
	}

	BasicCmdArgBuilder& operator += (target_string_view next_arg) {
		std::basic_string<_Elem, std::char_traits<_Elem>, std::allocator<_Elem>> _tmp;
		_tmp.reserve(next_arg.size() * 2);

		bool need_quote = false;
		intptr_t cnt_rslash = 0;
		for (_Elem c : next_arg) {
			if (c == '\0')
				break;

			switch (c) {
			case '\t':
			case ' ':
				need_quote = true;
				break;
			case '\"':
				while (cnt_rslash-- > 0)
					_tmp.append(1, '\\');
				_tmp.append(1, '\\');
				break;
			}

			_tmp.append(1, c);

			if (c == '\\')
				cnt_rslash++;
			else
				cnt_rslash = 0;
		}
		if (need_quote)
			while (cnt_rslash-- > 0)
				_tmp.append(1, '\\');

		_tmp.shrink_to_fit();

		this->_str.reserve(this->_str.size() + _tmp.size() + 5);

		if (!this->_str.empty())
			this->_str.append(1, ' ');
		if (need_quote)
			this->_str.append(1, '\"');
		this->_str.append(_tmp);
		if (need_quote)
			this->_str.append(1, '\"');

		return *this;
	}

	BasicCmdArgBuilder& operator += (const target_string& next_arg) {
		return this->operator+=(target_string_view{ next_arg });
	}
	BasicCmdArgBuilder& operator += (const _Elem* next_arg) {
		return this->operator+=(target_string_view{ next_arg });
	}

	BasicCmdArgBuilder& operator += (const BasicCmdArgBuilder& right) {
		this->_str.reserve(this->_str.size() + right._str.size() + 2);
		this->_str.append(1, ' ');
		this->_str.append(right._str);
		return *this;
	}

	operator target_string () const {
		return _str;
	}

	target_string get_result() const {
		return _str;
	}

private:
	target_string _str;
};

export template <typename _Elem>
BasicCmdArgBuilder<_Elem> operator + (const BasicCmdArgBuilder<_Elem>& left, typename BasicCmdArgBuilder<_Elem>::target_string_view next_arg) {
	BasicCmdArgBuilder<_Elem> res{ left };
	res += next_arg;
	return std::move(res);
}
export template <class _Elem>
BasicCmdArgBuilder<_Elem> operator + (const BasicCmdArgBuilder<_Elem>& left, const std::basic_string<_Elem, std::char_traits<_Elem>, std::allocator<_Elem>>& next_arg) {
	return left + std::basic_string_view<_Elem>{next_arg};
}
export template <class _Elem>
BasicCmdArgBuilder<_Elem> operator + (const BasicCmdArgBuilder<_Elem>& left, const _Elem* next_arg) {
	return left + std::basic_string_view<_Elem>{next_arg};
}
export template <class _Elem>
BasicCmdArgBuilder<_Elem> operator + (const BasicCmdArgBuilder<_Elem>& left, const BasicCmdArgBuilder<_Elem>& right) {
	BasicCmdArgBuilder<_Elem> res{ left };
	res += right;
	return res;
}

export template <class _Elem>
std::ostream& operator << (std::ostream& left, const BasicCmdArgBuilder<_Elem>& right) {
	return left << right._str;
}

export template <class _Elem>
std::wostream& operator << (std::wostream& left, const BasicCmdArgBuilder<_Elem>& right) {
	return left << right._str;
}

export using CmdArgBuilderA = BasicCmdArgBuilder<char>;
export using CmdArgBuilderW = BasicCmdArgBuilder<wchar_t>;
