export module TempGuardModule;

/**
 * @brief 临时获取所有权，并保护目标的旧有值，析构时自动还原旧值。
 * @tparam _T: 任何可以无异常复制的类型。
*/
export 
template<class _T>
class TempGuard final {
public:
	/**
	 * @brief 构造函数，自动复制保留旧值。
	 * @param t: 目标对象。
	*/
	explicit TempGuard(_T& t) noexcept :
		r_target(t) {
		m_old = t;
	}
	/**
	 * @brief 析构函数，自动还原旧值。
	*/
	~TempGuard() noexcept {
		r_target = m_old;
	}

	TempGuard(const TempGuard&) = delete;
	TempGuard& operator=(const TempGuard&) = delete;

	/**
	 * @brief 临时修改。把目标的值变为n。
	 * @param n: 临时值。
	*/
	void operator=(const _T& n) noexcept {
		r_target = n;
	}
	/**
	 * @brief 临时修改。把目标的值变为n。
	 * @param n: 临时值。
	*/
	void operator=(_T&& n) noexcept {
		r_target = n;
	}

protected:
	_T& r_target;
	_T m_old;
}; // template class TempGuard
