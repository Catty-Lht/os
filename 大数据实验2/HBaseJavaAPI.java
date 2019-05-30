/*
 * 创建一个students表,并进行相关操作
 */
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
 
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.util.Bytes;
 
public class HBaseJavaAPI {
	// 声明静态配置
	private static Configuration conf = null;
 
	static {
		conf = HBaseConfiguration.create();
		conf.set("hbase.zookeeper.quorum", "192.168.56.121");
		conf.set("hbase.zookeeper.property.clientPort", "2181");
	}
 
	//判断表是否存在
	private static boolean isExist(String tableName) throws IOException {
		HBaseAdmin hAdmin = new HBaseAdmin(conf);
		return hAdmin.tableExists(tableName);
	}
 
	// 创建数据库表
	public static void createTable(String tableName, String[] columnFamilys)
			throws Exception {
		// 新建一个数据库管理员
		HBaseAdmin hAdmin = new HBaseAdmin(conf);
		if (hAdmin.tableExists(tableName)) {
			System.out.println("表 "+tableName+" 已存在！");
			//System.exit(0);
		} else {
			// 新建一个students表的描述
			HTableDescriptor tableDesc = new HTableDescriptor(tableName);
			// 在描述里添加列族
			for (String columnFamily : columnFamilys) {
				tableDesc.addFamily(new HColumnDescriptor(columnFamily));
			}
			// 根据配置好的描述建表
			hAdmin.createTable(tableDesc);
			System.out.println("创建表 "+tableName+" 成功!");
		}
	}
 
	// 删除数据库表
	public static void deleteTable(String tableName) throws Exception {
		// 新建一个数据库管理员
		HBaseAdmin hAdmin = new HBaseAdmin(conf);
		if (hAdmin.tableExists(tableName)) {
			// 关闭一个表
			hAdmin.disableTable(tableName);
			hAdmin.deleteTable(tableName);
			System.out.println("删除表 "+tableName+" 成功！");
		} else {
			System.out.println("删除的表 "+tableName+" 不存在！");
			System.exit(0);
		}
	}
 
	// 添加一条数据
	public static void addRecord(String tableName, String row,
			String columnFamily, String column, String value) throws Exception {
		HTable table = new HTable(conf, tableName);
		Put put = new Put(Bytes.toBytes(row));// 指定行
		// 参数分别:列族、列、值
		put.add(Bytes.toBytes(columnFamily), Bytes.toBytes(column),
				Bytes.toBytes(value));
		table.put(put);
	}
 
	// 删除一条(行)数据
	public static void deleteRow(String tableName, String row) throws Exception {
		HTable table = new HTable(conf, tableName);
		Delete del = new Delete(Bytes.toBytes(row));
		table.delete(del);
	}
 
	// 删除多条数据
	public static void delMultiRows(String tableName, String[] rows)
			throws Exception {
		HTable table = new HTable(conf, tableName);
		List<Delete> delList = new ArrayList<Delete>();
		for (String row : rows) {
			Delete del = new Delete(Bytes.toBytes(row));
			delList.add(del);
		}
		table.delete(delList);
	}
 
	// 获取一条数据
	public static void scanColumn(String tableName, String row) throws Exception {
		HTable table = new HTable(conf, tableName);
		Get get = new Get(Bytes.toBytes(row));
		Result result = table.get(get);
		// 输出结果,raw方法返回所有keyvalue数组
		for (KeyValue rowKV : result.raw()) {
			System.out.print("行名:" + new String(rowKV.scanColumn()) + " ");
			System.out.print("时间戳:" + rowKV.getTimestamp() + " ");
			System.out.print("列族名:" + new String(rowKV.getFamily()) + " ");
			System.out.print("列名:" + new String(rowKV.getQualifier()) + " ");
			System.out.println("值:" + new String(rowKV.getValue()));
		}
	}
 
	// 获取所有数据
	public static void getAllRows(String tableName) throws Exception {
		HTable table = new HTable(conf, tableName);
		Scan scan = new Scan();
		ResultScanner results = table.getScanner(scan);
		// 输出结果
		for (Result result : results) {
			for (KeyValue rowKV : result.raw()) {
				System.out.print("行名:" + new String(rowKV.scanColumn()) + " ");
				System.out.print("时间戳:" + rowKV.getTimestamp() + " ");
				System.out.print("列族名:" + new String(rowKV.getFamily()) + " ");
				System.out
						.print("列名:" + new String(rowKV.getQualifier()) + " ");
				System.out.println("值:" + new String(rowKV.getValue()));
			}
		}
	}
 
	// 主函数
	public static void main(String[] args) {
		try {
			String tableName = "student";
			// 第一步：创建数据库表：“student”
			String[] columnFamilys = { "information", "score" };
		    HBaseJavaAPI.createTable(tableName, columnFamilys);
			// 第二步：向数据表的添加数据
			// 添加第一行数据
			if (isExist(tableName)) {
				HBaseJavaAPI.addRecord(tableName, "2015000", "information", "name", "dyk");
				HBaseJavaAPI.addRecord(tableName, "2015000", "information", "age", "20");
				HBaseJavaAPI.addRecord(tableName, "2015000", "information", "sex", "boy");
				HBaseJavaAPI.addRecord(tableName, "2015000", "score", "123001", "97");
				HBaseJavaAPI.addRecord(tableName, "2015000", "score", "123002", "128");
				HBaseJavaAPI.addRecord(tableName, "2015000", "score", "123003", "85");
				// 添加第二行数据
				HBaseJavaAPI.addRecord(tableName, "2015001", "information", "name", "zhangsan");
				HBaseJavaAPI.addRecord(tableName, "2015001", "information", "age", "23");
				HBaseJavaAPI.addRecord(tableName, "2015001", "information", "sex", "male");
				HBaseJavaAPI.addRecord(tableName, "2015001", "score", "123001","86");
				HBaseJavaAPI.addRecord(tableName, "2015001", "score", "123003","69");
				//HBaseJavaAPI.addRecord(tableName, "2015001", "score", "123002","90");
				// 添加第三行数据
				HBaseJavaAPI.addRecord(tableName, "2015002", "information", "name", "Mary");
				HBaseJavaAPI.addRecord(tableName, "2015002", "information", "age", "22");
				HBaseJavaAPI.addRecord(tableName, "2015002", "information", "sex","female");
				HBaseJavaAPI.addRecord(tableName, "2015002", "score", "123002","77");
				HBaseJavaAPI.addRecord(tableName, "2015002", "score", "123003","99");
				//HBaseJavaAPI.addRecord(tableName, "2015002", "score", "english","99");
				HBaseJavaAPI.addRecord(tableName, "2015003", "information", "name", "Lisi");
				HBaseJavaAPI.addRecord(tableName, "2015003", "information", "age", "24");
				HBaseJavaAPI.addRecord(tableName, "2015003", "information", "sex","male");
				HBaseJavaAPI.addRecord(tableName, "2015003", "score", "123001","98");
				HBaseJavaAPI.addRecord(tableName, "2015003", "score", "123002","95");
				// 第三步：获取一条数据
				System.out.println("**************获取一条(2015000)数据*************");
				HBaseJavaAPI.scanColumn(tableName, "2015000");
				// 第四步：获取所有数据
				System.out.println("**************获取所有数据***************");
				HBaseJavaAPI.getAllRows(tableName);
 
				// 第五步：删除一条数据
				System.out.println("************删除一条(2015000)数据************");
				HBaseJavaAPI.deleteRow(tableName, "2015000");
				HBaseJavaAPI.getAllRows(tableName);
				// 第六步：删除多条数据
				//System.out.println("**************删除多条数据***************");
				//String rows[] = new String[] { "qingqing","xiaoxue" };
				//HBaseJavaAPI.delMultiRows(tableName, rows);
				//HBaseJavaAPI.getAllRows(tableName);
				// 第七步：删除数据库
				//System.out.println("***************删除数据库表**************");
				//HBaseJavaAPI.deleteTable(tableName);
				//System.out.println("表"+tableName+"存在吗？"+isExist(tableName));
			} else {
				System.out.println(tableName + "此数据库表不存在！");
			}
 
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
 
}