import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import kafka.consumer.Consumer;
import kafka.consumer.ConsumerConfig;
import kafka.consumer.ConsumerIterator;
import kafka.consumer.KafkaStream;
import kafka.javaapi.consumer.ConsumerConnector;
 
public class MyConsumer {
 
	public static void main(String[] args) {
		File fp = new File("testdata1.txt");
		static FileWriter fw = null;
		try {
			if (!fp.exists()) {
				fp.createNewFile(); // 创建输出的中间文件
			}
			fw = new FileWriter(fp);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	
		String topic = "mykafka";
		
		ConsumerConnector consumer = Consumer.createJavaConsumerConnector(createConsumerConfig()); 
		Map<String, Integer> topicCountMap = new HashMap<String, Integer>();
		topicCountMap.put(topic, new Integer(1));
		Map<String, List<KafkaStream<byte[], byte[]>>> consumerMap = consumer.createMessageStreams(topicCountMap);
		KafkaStream<byte[], byte[]> stream =  consumerMap.get(topic).get(0);
		ConsumerIterator<byte[], byte[]> it = stream.iterator();
	    while(it.hasNext())
		{try {
			fw.write(new String(it.next().message()));
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
			
			System.out.println("consume: " + new String(it.next().message()));
		}
		
	}
	
	private static ConsumerConfig createConsumerConfig() {
	    Properties props = new Properties();
	    props.put("group.id","group1");
	    props.put("zookeeper.connect","cluster1:2181,cluster2:2181,cluster3:2181");
	    props.put("zookeeper.session.timeout.ms", "400");
	    props.put("zookeeper.sync.time.ms", "200");
	    props.put("auto.commit.interval.ms", "1000");
	    return new ConsumerConfig(props);
	  }
}