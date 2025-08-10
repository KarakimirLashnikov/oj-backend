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
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus
mkdir build
cd build

- 配置 CMake
cmake ..

- 编译和安装
make -j$(nproc)
sudo make install

- 通过 apt 安装的 hiredis 没有 cmake 配置文件，修改 redis++-config.cmake
```cmake
get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

# 手动查找 hiredis
find_path(Hiredis_INCLUDE_DIR
  NAMES hiredis.h
  PATHS /usr/include/hiredis
)

find_library(Hiredis_LIBRARY
  NAMES libhiredis
  PATHS /usr/lib/x86_64-linux-gnu
)

if(Hiredis_INCLUDE_DIR AND Hiredis_LIBRARY)
  message(STATUS "Found Hiredis: ${Hiredis_LIBRARY}")
  # 创建 INTERFACE 库目标
  add_library(hiredis INTERFACE IMPORTED)
  target_include_directories(hiredis INTERFACE ${Hiredis_INCLUDE_DIR})
  target_link_libraries(hiredis INTERFACE ${Hiredis_LIBRARY})
else()
  message(FATAL_ERROR "Hiredis not found! Install via: sudo apt-get install libhiredis-dev")
endif()

# 包含 redis++ 目标文件
include("${CMAKE_CURRENT_LIST_DIR}/redis++-targets.cmake")

# 修复 redis++ 目标的链接依赖
get_target_property(OLD_LINK_LIBS redis++::redis++ INTERFACE_LINK_LIBRARIES)
if(OLD_LINK_LIBS MATCHES "libhiredis\\.so")
  set_target_properties(redis++::redis++ PROPERTIES
    INTERFACE_LINK_LIBRARIES "hiredis::hiredis"
  )
  message(STATUS "Updated redis++ link dependencies to use hiredis::hiredis")
endif()

check_required_components(redis++)
```
---

## API 文档
### 1. 提交管理 `/submissions`
#### 提交代码
**Endpoint**  
`POST {base_url}/api/submissions/submit`

**请求参数**  
| 字段          | 类型     | 必填 | 说明                     |
|---------------|----------|------|--------------------------|
| username     | string   | ✓    | 用户名（需已存在）       |
| problem_title | string   | ✓    | 题目名称（需已存在）     |
| source_code   | string   | ✓    | 源代码                   |
| language_id   | integer  | ✓    | 语言ID（对应编程语言类型）|
| compile_options| array    | ✗    | 编译选项数组（字符串元素）|

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
{"status": "success", "message": "login succeed"}

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
`POST {base_url}/api/problems/create`

**请求参数**  
| 字段            | 类型    | 必填 | 说明                     |
|-----------------|---------|------|--------------------------|
| title           | string  | ✓    | 题目标题                 |
| time_limit_s    | float   | ✓    | 时间限制（秒）           |
| memory_limit_kb | integer | ✓    | 内存限制（KB）           |
| stack_limit_kb  | integer | ✓    | 栈限制（KB）             |
| difficulty      | integer | ✓    | 难度等级（对应DifficultyLevel枚举）|
| description     | string  | ✓    | 题目描述                 |
| extra_time_s    | float   | ✗    | 额外时间（秒，默认0.5）  |
| wall_time_s     | float   | ✗    | 挂钟时间（秒，默认time_limit_s + 1.0）|

**响应示例**  
```json
// 成功 (HTTP 200)
{"status": "success", "message": "<title> created successfully"}

// 参数错误 (HTTP 400)
{"status": "failure", "message": "Missing parameters"}

// 系统错误 (HTTP 500)
{"status": "failure", "message": "数据库操作异常信息"}
```

#### 更新题目
**Endpoint**  
`POST {base_url}/api/problems/update`  

**请求参数**：同创建题目接口（需确保题目已存在）

**响应示例**  
```json
// 成功 (HTTP 200)
{"status": "success", "message": "<title> updated successfully"}

// 参数错误 (HTTP 400)
{"status": "failure", "message": "Problem <title> does not exist"}
```

#### 上传测试用例
**Endpoint**  
`POST {base_url}/api/problems/upload_test_cases`

**请求参数**  
| 字段          | 类型  | 必填 | 说明                          |
|---------------|-------|------|-------------------------------|
| problem_title | string| ✓    | 关联的题目名称（需已存在）    |
| test_cases    | array | ✓    | 测试用例数组                  |

**测试用例结构**  
```json
{
  "stdin": "输入数据字符串",
  "expected_output": "预期输出字符串",
  "sequence": 1,  // 排序标识（整数）
  "hidden": false  // 可选，是否为隐藏用例（默认false）
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