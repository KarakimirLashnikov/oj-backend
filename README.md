我将优化文档以提高可读性和实用性，主要改进配置步骤的清晰度和API文档的完整性：

### 优化后的README.md

```markdown
## 环境配置
### 依赖库安装
```bash
# 克隆第三方库
cd third_party
git clone git@github.com:yhirose/cpp-httplib.git
git clone git@github.com:nlohmann/json.git
git clone git@github.com:gabime/spdlog.git

# 安装系统依赖
sudo apt update
sudo apt install -y \
  libboost-all-dev \
  libseccomp-dev libseccomp2 seccomp \
  flex bison \
  mysql-server libmysqlclient-dev \
  libmysqlcppconn-dev \
  libsodium-dev
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
| user_name     | string   | ✓    | 用户名                   |
| problem_title | string   | ✓    | 题目名称（需已存在）     |
| source_code   | string   | ✓    | 源代码                   |
| language_id   | string   | ✓    | 语言ID（当前仅支持1: C++）|
| compile_options| array    | ✗    | 编译选项数组             |

**响应示例**  
```json
// 成功 (HTTP 200)
{
  "status": "success",
  "message": "Submission received",
  "submission_id": "<KEY>" // boost::uuid::to_string, length == 36
}

// 失败 (HTTP 400)
{
  "status": "failure",
  "message": "Problem not found"
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
| user_name | string | ✓    | 用户名       |
| password  | string | ✓    | 密码         |

**响应**  
```json
// 成功
{"status": "success", "message": "Login successful"}

// 失败
{"status": "failure", "message": "Invalid credentials"}
```

#### 用户注册
**Endpoint**  
`POST {base_url}/api/login/signup`

**请求参数**  
| 字段      | 类型   | 必填 | 约束          |
|-----------|--------|------|---------------|
| user_name | string | ✓    | 唯一用户名    |
| password  | string | ✓    | 密码          |
| email     | string | ✓    | 有效邮箱格式  |

**响应**  
```json
{"status": "success", "message": "User created"}
```

---

### 3. 题目管理 `/problems`
#### 创建题目
**Endpoint**  
`POST {base_url}/api/problems/create`

**请求参数**  
| 字段            | 类型    | 必填 | 约束                     |
|-----------------|---------|------|--------------------------|
| title           | string  | ✓    | 标题长度 < 128字符       |
| time_limit_s    | float   | ✓    | 时间限制(秒)             |
| memory_limit_kb | integer | ✓    | 内存限制(KB)             |
| stack_limit_kb  | integer | ✓    | 栈限制(KB)               |
| difficulty      | integer | ✓    | 难度等级(1-5)            |
| description     | string  | ✓    | 题目描述                 |

**响应**  
```json
{"status": "success", "message": "Problem created"}
```

#### 更新题目
**Endpoint**  
`POST {base_url}/api/problems/update`  
*参数同创建接口，需确保题目已存在*

#### 上传测试用例
**Endpoint**  
`POST {base_url}/api/problems/upload_test_cases`

**请求参数**  
| 字段          | 类型  | 必填 | 说明                          |
|---------------|-------|------|-------------------------------|
| problem_title | string| ✓    | 关联的题目名称                |
| test_cases    | array | ✓    | 测试用例数组                  |

**测试用例结构**  
```json
{
  "stdin": "输入数据",
  "expected_stdout": "预期输出",
  "sequence": 排序标识,  // 使用整数序列
  "is_hidden": true      // 可选，是否隐藏用例
}
```

**响应**  
```json
{"status": "success", "message": "Test cases added"}
```