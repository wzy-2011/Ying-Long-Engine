#ifndef YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "parser.h"
#include "emitter.h"
#include "emitterstyle.h"
#include "stlemitter.h"
#include "exceptions.h"

#include "node/node.h"
#include "node/impl.h"
#include "node/convert.h"
#include "node/iterator.h"
#include "node/detail/impl.h"
#include "node/parse.h"
#include "node/emit.h"
#include <DirectXMath.h>
using namespace DirectX;

namespace YAML {
	template <>
	struct convert<XMFLOAT3>
	{
		static Node encode(const XMFLOAT3& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		// 为方便起见也实现解码方法
		static bool decode(const Node& node, XMFLOAT3& rhs) {
			if (!node.IsSequence() || node.size() != 3) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template <>
	struct convert<XMFLOAT4>
	{
		static Node encode(const XMFLOAT4& rhs) {
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, XMFLOAT4& rhs) {
			if (!node.IsSequence() || node.size() != 4) {
				return false;
			}
			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};
}
namespace YAML {
	inline Emitter& operator<<(Emitter& out, const XMFLOAT3& v) {
		out << Flow;
		out << BeginSeq << v.x << v.y << v.z << EndSeq;
		return out;
	}
	inline Emitter& operator<<(Emitter& out, const XMFLOAT4& v) {
		out << Flow;
		out << BeginSeq << v.x << v.y << v.z << v.w << EndSeq;
		return out;
	}
}

#endif  // YAML_H_62B23520_7C8E_11DE_8A39_0800200C9A66
