```markdown
## 环境配置
### 依赖库安装
```bash
# 克隆第三方库
cd third_party
git clone git@github.com:yhirose/cpp-httplib.git
git clone git@github.com:nlohmann/json.git
git clone git@github.com:gabime/spdlog.git
git clone git@github.com:sewenew/redis-plus-plus.git

# 安装系统依赖
sudo apt update
sudo apt install -y \
  pkg-config \
  libboost-all-dev \
  libseccomp-dev libseccomp2 seccomp \
  flex bison \
  mysql-server libmysqlclient-dev \
  libmysqlcppconn-dev \
  libsodium-dev \
  redis-server \
  libhiredis-dev
```

### MySQL配置说明
 安装后需修改数据库配置[CMakeLists](./database/CMakeLists.txt)：
```cmake
# 检查并更新 MySQL Connector/C++ 相关路径:
# 1. 查找 MySQL Connector/C++ 头文件路径
find_path(MYSQLCPPCONN_INCLUDE_DIR NAMES mysql_connection.h PATHS
    /usr/include/mysql
)

# 2. 查找 MySQL Connector/C++ 库文件
find_library(MYSQLCPPCONN_LIBRARY NAMES mysqlcppconn PATHS
    /usr/lib/x86_64-linux-gnu
)

if(MYSQLCPPCONN_INCLUDE_DIR AND MYSQLCPPCONN_LIBRARY)
    message(STATUS "Found MySQL Connector/C++: ${MYSQLCPPCONN_LIBRARY}")
    add_library(mysqlcppconn INTERFACE IMPORTED)
    target_include_directories(mysqlcppconn INTERFACE ${MYSQLCPPCONN_INCLUDE_DIR})
    target_link_libraries(mysqlcppconn INTERFACE ${MYSQLCPPCONN_LIBRARY})
else()
    message(FATAL_ERROR "MySQL Connector/C++ not found!")
endif()
```

### redis++配置说明
- 通过 apt 安装的 hiredis 没有 cmake 配置文件，修改 redis-plus-plus 中的 redis++-config.cmake.in
```cmake
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

# 解析依赖列表（原逻辑）
string(REPLACE "," ";" REDIS_PLUS_PLUS_DEPENDS_LIST @REDIS_PLUS_PLUS_DEPENDS@)

# 手动处理hiredis和hiredis_ssl，其他依赖仍用find_dependency
foreach(REDIS_PLUS_PLUS_DEP ${REDIS_PLUS_PLUS_DEPENDS_LIST})
    # 处理hiredis（无CMake配置文件，手动查找）
    if(REDIS_PLUS_PLUS_DEP STREQUAL "hiredis")
        # 查找hiredis头文件
        find_path(HIREDIS_INCLUDE_DIR
            NAMES hiredis.h
            PATHS /usr/include/hiredis
            DOC "hiredis header directory"
        )
        # 查找hiredis库文件
        find_library(HIREDIS_LIBRARY
            NAMES hiredis
            PATHS /usr/lib/x86_64-linux-gnu
            DOC "hiredis library file"
        )
        # 验证查找结果
        if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
            message(FATAL_ERROR "hiredis not found. Install via: sudo apt install libhiredis-dev")
        endif()
        # 设置hiredis变量（模拟CMake配置文件输出）
        set(hiredis_FOUND TRUE)
        set(hiredis_INCLUDE_DIRS ${HIREDIS_INCLUDE_DIR})
        set(hiredis_LIBRARIES ${HIREDIS_LIBRARY})

    # 处理hiredis_ssl（TLS支持，同样手动查找）
    elseif(REDIS_PLUS_PLUS_DEP STREQUAL "hiredis_ssl")
        # 查找hiredis_ssl头文件
        find_path(HIREDIS_SSL_INCLUDE_DIR
            NAMES ssl.h
            PATHS /usr/includehiredis
            DOC "hiredis_ssl header directory"
        )
        # 查找hiredis_ssl库文件
        find_library(HIREDIS_SSL_LIBRARY
            NAMES hiredis_ssl
            PATHS /usr/lib/x86_64-linux-gnu
            DOC "hiredis_ssl library file"
        )
        # 验证查找结果
        if(NOT HIREDIS_SSL_INCLUDE_DIR OR NOT HIREDIS_SSL_LIBRARY)
            message(FATAL_ERROR "hiredis_ssl not found. Install libhiredis-dev with SSL support.")
        endif()
        # 设置hiredis_ssl变量
        set(hiredis_ssl_FOUND TRUE)
        set(hiredis_ssl_INCLUDE_DIRS ${HIREDIS_SSL_INCLUDE_DIR})
        set(hiredis_ssl_LIBRARIES ${HIREDIS_SSL_LIBRARY})

    # 其他依赖（如有）仍使用find_dependency
    else()
        find_dependency(${REDIS_PLUS_PLUS_DEP} REQUIRED)
    endif()
endforeach()

# 引入目标文件（原逻辑保留）
include("${CMAKE_CURRENT_LIST_DIR}/redis++-targets.cmake")

check_required_components(redis++)
```
- 构建
cd redis-plus-plus
mkdir build
cd build

- 配置 CMake
cmake ..

- 编译和安装
make -j$(nproc)
sudo make install
---

## API 文档
### 1. 提交管理 `/submissions`
#### 提交代码
**Endpoint**  
`POST {base_url}/api/submissions/submit`

**请求参数**  
| 字段          | 类型     | 必填 | 说明                     |
|---------------|----------|------|--------------------------|
| token     | string   | ✓    | 用户令牌        |
| problem_title | string   | ✓    | 题目名称（需已存在）     |
| source_code   | string   | ✓    | 源代码                   |
| language_id   | integer  | ✓    | 语言ID（1:C++/2:Python）|
| compile_options| array    | ✗    | 编译选项数组（字符串列表）|

**响应示例**  
```json
// 成功 (HTTP 200)
{
  "status": "success",
  "message": "submission succeed",
  "submission_id": "<UUID>" // boost::uuid生成的36位字符串
}

// 参数错误 (HTTP 400)
{
  "status": "failure",
  "message": "Missing parameters"
}

// 系统错误 (HTTP 500)
{
  "status": "failure",
  "message": "judge submission error"
}
```

---

### 2. 用户认证 `/login`
#### 用户登录
**Endpoint**  
`POST {base_url}/api/login/login`

**请求参数**  
| 字段      | 类型   | 必填 | 说明         |
|-----------|--------|------|--------------|
| username | string | ✓    | 用户名       |
| password  | string | ✓    | 密码         |

**响应示例**  
```json
// 成功 (HTTP 200)
{"status": "success", "message": "login succeed", "token": "<jwt token>"}

// 参数错误 (HTTP 400)
{"status": "failure", "message": "username doesn't exist"}

// 系统错误 (HTTP 500)
{"status": "failure", "message": "数据库操作异常信息"}
```

#### 用户注册
**Endpoint**  
`POST {base_url}/api/login/signup`

**请求参数**  
| 字段      | 类型   | 必填 | 约束          |
|-----------|--------|------|---------------|
| username | string | ✓    | 唯一用户名    |
| password  | string | ✓    | 密码（将被加密存储）|
| email     | string | ✓    | 邮箱地址      |

**响应示例**  
```json
// 成功 (HTTP 200)
{"status": "success", "message": "signup succeed"}

// 参数错误 (HTTP 400)
{"status": "failure", "message": "username already exist"}

// 系统错误 (HTTP 500)
{"status": "failure", "message": "crypto_pwhash_str failed"}
```

---

### 3. 题目管理 `/problems`
#### 创建题目
**Endpoint**  
`POST {base_url}/api/problems/create_problem`

**请求参数**  
| 字段            | 类型    | 必填 | 说明                     |
|-----------------|---------|------|--------------------------|
| token | string  | ✓    | 用户令牌                  |
| title           | string  | ✓    | 题目标题                 |
| description     | string  | ✓    | 题目描述                 |
| difficulty      | string | ✓    | 题目难度 (Beginner/Basic/Intermediate/Advanced/Expert) |

**响应示例**  
```json
// 成功 (HTTP 200)
{"status": "success", "message": "create succeed"}

// 参数错误 (HTTP 400)
{"status": "failure", "message": "Missing parameters"}

// 系统错误 (HTTP 500)
{"status": "failure", "message": "数据库操作异常信息"}
```

#### 上传测试用例
**Endpoint**  
`POST {base_url}/api/problems/upload_test_cases`

**请求参数**  
| 字段          | 类型  | 必填 | 说明                          |
|---------------|-------|------|-------------------------------|
| token     | string | ✓    | 用户令牌                      |
| problem_title | string| ✓    | 关联的题目名称（需已存在）    |
| test_cases    | array | ✓    | 测试用例数组                  |

**测试用例结构**  
```json
{
  "stdin": "输入数据字符串",
  "expected_output": "预期输出字符串",
  "sequence": 1,  // 排序标识（整数，从1开始）
}
```

**响应示例**  
```json
// 成功 (HTTP 200)
{"status": "success", "message": "Test cases successfully uploaded for <title>"}

// 参数错误 (HTTP 400)
{"status": "failure", "message": "Missing test cases"}

// 系统错误 (HTTP 500)
{"status": "failure", "message": "upload test cases failed"}
```


#### 上传题目限制
**Endpoint**  
`POST {base_url}/api/problems/add_problem_limit`

**请求参数**  
| 字段          | 类型  | 必填 | 说明                          |
|---------------|-------|------|-------------------------------|
| token     | string | ✓    | 用户令牌                      |
| problem_title | string| ✓    | 关联的题目名称（需已存在）    |
| language_id    | integer | ✓    | 语言ID (1:C++/2:Python)|
| time_limit_s | float | ✓    | 时限，单位：秒               |
| memory_limit_kb | integer | ✓    | 时限，单位：KB    |
| extra_time_s | float | ✗    | 每个测试用额外时间，单位：秒|
| wall_time_s | float | ✗    | 允许运行时间，单位：秒|


**响应示例**  
```json
// 成功 (HTTP 200)
{"status": "success", "message": "add limits succeed"}

// 参数错误 (HTTP 400)
{"status": "failure", "message": "Missing test cases"}

// 系统错误 (HTTP 500)
{"status": "failure", "message": "error"}
```