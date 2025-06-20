import React, { useState, useEffect, useRef } from 'react';
import { Send, Paperclip, Smile, MoreVertical } from 'lucide-react';
import { format } from 'date-fns';

interface Message {
  id: string;
  content: string;
  sender: string;
  timestamp: Date;
  isOwn: boolean;
  type: 'text' | 'file' | 'image';
}

interface ChatWindowProps {
  chatId: string;
}

const ChatWindow: React.FC<ChatWindowProps> = ({ chatId }) => {
  const [messages, setMessages] = useState<Message[]>([]);
  const [newMessage, setNewMessage] = useState('');
  const [isTyping, setIsTyping] = useState(false);
  const messagesEndRef = useRef<HTMLDivElement>(null);

  // Mock data - replace with actual WebSocket connection
  useEffect(() => {
    const mockMessages: Message[] = [
      {
        id: '1',
        content: 'Hey! How are you doing?',
        sender: 'john@cockpit.com',
        timestamp: new Date(Date.now() - 3600000),
        isOwn: false,
        type: 'text',
      },
      {
        id: '2',
        content: 'I\'m doing great! Just finished the project we discussed.',
        sender: 'me',
        timestamp: new Date(Date.now() - 3000000),
        isOwn: true,
        type: 'text',
      },
      {
        id: '3',
        content: 'That\'s awesome! Can you share the details?',
        sender: 'john@cockpit.com',
        timestamp: new Date(Date.now() - 2400000),
        isOwn: false,
        type: 'text',
      },
      {
        id: '4',
        content: 'Sure! I\'ll send you the documentation.',
        sender: 'me',
        timestamp: new Date(Date.now() - 1800000),
        isOwn: true,
        type: 'text',
      },
    ];
    setMessages(mockMessages);
  }, [chatId]);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  };

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const handleSendMessage = () => {
    if (newMessage.trim()) {
      const message: Message = {
        id: Date.now().toString(),
        content: newMessage,
        sender: 'me',
        timestamp: new Date(),
        isOwn: true,
        type: 'text',
      };
      setMessages(prev => [...prev, message]);
      setNewMessage('');
      
      // TODO: Send message via WebSocket
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSendMessage();
    }
  };

  return (
    <div className="flex-1 flex flex-col">
      {/* Chat Header */}
      <div className="bg-dark-900 border-b border-dark-700 p-4">
        <div className="flex items-center justify-between">
          <div className="flex items-center space-x-3">
            <div className="w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center">
              <span className="text-white font-medium text-sm">J</span>
            </div>
            <div>
              <h2 className="text-lg font-semibold text-white">john@cockpit.com</h2>
              <p className="text-sm text-dark-400">
                {isTyping ? 'Typing...' : 'Online'}
              </p>
            </div>
          </div>
          <button className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors">
            <MoreVertical className="w-5 h-5" />
          </button>
        </div>
      </div>

      {/* Messages Area */}
      <div className="flex-1 overflow-y-auto p-4 space-y-4 chat-animated-bg">
        {messages.map((message) => (
          <div
            key={message.id}
            className={`flex ${message.isOwn ? 'justify-end' : 'justify-start'}`}
          >
            <div className={`message-bubble ${
              message.isOwn ? 'message-bubble-sent' : 'message-bubble-received'
            }`}>
              <p className="text-sm">{message.content}</p>
              <p className={`text-xs mt-1 ${
                message.isOwn ? 'text-white/70' : 'text-dark-300'
              }`}>
                {format(message.timestamp, 'HH:mm')}
              </p>
            </div>
          </div>
        ))}
        <div ref={messagesEndRef} />
      </div>

      {/* Message Input */}
      <div className="bg-dark-900 border-t border-dark-700 p-4">
        <div className="flex items-end space-x-3">
          <div className="flex-1 relative">
            <textarea
              value={newMessage}
              onChange={(e) => setNewMessage(e.target.value)}
              onKeyPress={handleKeyPress}
              placeholder="Type a message..."
              className="input-field w-full resize-none min-h-[44px] max-h-32 py-3 pr-20"
              rows={1}
            />
            <div className="absolute right-2 bottom-2 flex items-center space-x-1">
              <button className="p-1 text-dark-400 hover:text-white transition-colors">
                <Paperclip className="w-4 h-4" />
              </button>
              <button className="p-1 text-dark-400 hover:text-white transition-colors">
                <Smile className="w-4 h-4" />
              </button>
            </div>
          </div>
          <button
            onClick={handleSendMessage}
            disabled={!newMessage.trim()}
            className="btn-primary p-3 disabled:opacity-50 disabled:cursor-not-allowed"
          >
            <Send className="w-5 h-5" />
          </button>
        </div>
      </div>
    </div>
  );
};

export default ChatWindow; 